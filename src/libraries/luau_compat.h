#pragma once

// Poison system lua headers
#define lua_h
#define lualib_h

// These macros must be defined AFTER lua.h includes, so we hook them
// via -include which runs before source but we undef+redefine after lua.h
// by letting lua.h define them first, then we override below.
// lua.h guard is already set above for system lua.h only.