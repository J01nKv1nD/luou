#ifndef LOVE_LUA_HPP
#define LOVE_LUA_HPP

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "luacode.h"
}

#define LUA_H
#define LAUXLIB_H
#define LUALIB_H
#define LUACONF_H

#define luaL_ref(L, i) lua_ref(L, i)
#define luaL_unref(L, i, r) lua_unref(L, r)
#define lua_rawlen(L, i) lua_objlen(L, i)
#define luaL_newstate() lua_newstate(NULL, NULL)
#define lua_open() luaL_newstate()

#undef lua_pushcfunction
#define lua_pushcfunction(L, fn) lua_pushcclosurek(L, fn, #fn, 0, NULL)

#undef lua_pushcclosure
#define lua_pushcclosure(L, fn, n) lua_pushcclosurek(L, fn, #fn, n, NULL)

#undef luaL_error
#define luaL_error(L, ...) (lua_error(L), 0)

typedef luaL_Buffer luaL_Buffer_53;
#define lua53_str_pack(L) (lua_error(L), 0)
#define lua53_str_unpack(L) (lua_error(L), 0)

#ifdef __cplusplus
inline int luau_load_wrapper(lua_State* L, const char* chunk, size_t sz, const char* name) {
    size_t bsize = 0;
    char* bytecode = luau_compile(chunk, sz, NULL, &bsize);
    if (bytecode == NULL) return 1;
    int result = luau_load(L, name, bytecode, bsize, 0);
    free(bytecode);
    return result;
}
#undef luaL_loadbuffer
#define luaL_loadbuffer(L, s, sz, n) luau_load_wrapper(L, s, sz, n)
#endif

#endif