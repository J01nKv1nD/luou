#ifndef LOVE_LUA_HPP
#define LOVE_LUA_HPP

#ifndef LUA_COMPAT_ALL
#define LUA_COMPAT_ALL
#endif

// We are pointing directly to the Luau headers we moved
#ifdef __cplusplus
extern "C" {
#endif
	#include "lua.h"
	#include "lualib.h"
#ifdef __cplusplus
}
#endif

#endif // LOVE_LUA_HPP
