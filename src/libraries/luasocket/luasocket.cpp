/**
 * Copyright (c) 2006-2026 LOVE Development Team
 * ...
 **/

#include "luasocket.h"

// LuaSocket - use Luau headers, NOT system lua headers.
// These C files will also transitively include lua.h; ensure your CMake
// include_directories puts the Luau VM/include path BEFORE /usr/include.
extern "C" {
#include "libluasocket/luasocket.h"
#include "libluasocket/mime.h"
#include "libluasocket/unix.h"
}

// Lua files (embedded bytecode/source blobs)
#include "libluasocket/ftp.lua.h"
#include "libluasocket/headers.lua.h"
#include "libluasocket/http.lua.h"
#include "libluasocket/ltn12.lua.h"
#include "libluasocket/mbox.lua.h"
#include "libluasocket/mime.lua.h"
#include "libluasocket/smtp.lua.h"
#include "libluasocket/socket.lua.h"
#include "libluasocket/tp.lua.h"
#include "libluasocket/url.lua.h"

// Luau compiler for source->bytecode compilation
#include "luacode.h"

static void preload(lua_State *L, const char *name, lua_CFunction func)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushcfunction(L, func);
    lua_setfield(L, -2, name);
    lua_pop(L, 2);
}

// Helper: compile Lua source to Luau bytecode and load it.
// Returns 0 on success (chunk on top of stack), non-zero on error
// (error message string on top of stack).
static int luau_loadbuffer(lua_State *L, const char *source, size_t size, const char *chunkname)
{
    size_t bytecodeSize = 0;
    char *bytecode = luau_compile(source, size, nullptr, &bytecodeSize);
    if (!bytecode)
    {
        lua_pushstring(L, "luau_compile returned null");
        return 1;
    }

    int result = luau_load(L, chunkname, bytecode, bytecodeSize, 0);
    free(bytecode);
    return result; // 0 = chunk on stack, non-zero = error string on stack
}

static void preload(lua_State *L, const char *name, const char *chunkname, const void *lua, size_t size)
{
    if (luau_loadbuffer(L, (const char *) lua, size, chunkname) != 0)
    {
        // Loading failed; wrap the error in a function that re-raises it
        // when the module is required.
        const char *errWrapSrc =
            "local name, msg = ... return function() error(name..\": \"..msg) end";
        size_t wrapBytecodeSize = 0;
        char *wrapBytecode = luau_compile(errWrapSrc, strlen(errWrapSrc), nullptr, &wrapBytecodeSize);

        if (wrapBytecode && luau_load(L, "=[luasocket error wrapper]", wrapBytecode, wrapBytecodeSize, 0) == 0)
        {
            free(wrapBytecode);
            // Stack: ... errMsg wrapFn
            lua_pushstring(L, name);  // arg1: module name
            lua_pushvalue(L, -3);     // arg2: original error message
            lua_call(L, 2, 1);        // -> deferred error function
            lua_remove(L, -2);        // remove original error message
        }
        else
        {
            free(wrapBytecode);
            // Last resort: just push a dummy function
            lua_pop(L, 1); // pop original error message
            lua_pushcfunction(L, [](lua_State *L) -> int {
                lua_pushstring(L, "luasocket: failed to load module and failed to load error wrapper");
                lua_error(L);
                return 0;
            });
        }
    }

    lua_getglobal(L, "package");
    lua_getfield(L, -1, "preload");
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, name);
    lua_pop(L, 3);
}

namespace love
{

namespace luasocket
{

int preload(lua_State *L)
{
    // Preload native C modules
    ::preload(L, "socket.core", luaopen_socket_core);
    ::preload(L, "socket.unix", luaopen_socket_unix);
    ::preload(L, "mime.core",   luaopen_mime_core);

    // Preload Lua source modules (compiled to Luau bytecode at load time)
    ::preload(L, "socket",         "=[socket \"socket.lua\"]",  socket_lua,  sizeof(socket_lua));
    ::preload(L, "socket.ftp",     "=[socket \"ftp.lua\"]",     ftp_lua,     sizeof(ftp_lua));
    ::preload(L, "socket.http",    "=[socket \"http.lua\"]",    http_lua,    sizeof(http_lua));
    ::preload(L, "ltn12",          "=[socket \"ltn12.lua\"]",   ltn12_lua,   sizeof(ltn12_lua));
    ::preload(L, "mime",           "=[socket \"mime.lua\"]",    mime_lua,    sizeof(mime_lua));
    ::preload(L, "socket.smtp",    "=[socket \"smtp.lua\"]",    smtp_lua,    sizeof(smtp_lua));
    ::preload(L, "socket.tp",      "=[socket \"tp.lua\"]",      tp_lua,      sizeof(tp_lua));
    ::preload(L, "socket.url",     "=[socket \"url.lua\"]",     url_lua,     sizeof(url_lua));
    ::preload(L, "socket.headers", "=[socket \"headers.lua\"]", headers_lua, sizeof(headers_lua));
    ::preload(L, "mbox",           "=[socket \"mbox.lua\"]",    mbox_lua,    sizeof(mbox_lua));

    return 0;
}

} // luasocket
} // love