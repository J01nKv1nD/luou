cmake_minimum_required(VERSION 3.16)
project(love-luau)

# find_package(Lua)
# find_package(LuaJIT)

# 1. Include our new Luau library
add_subdirectory(src/libraries/luau)

# 2. Force LÖVE to use our Luau instead of searching for LuaJIT
set(LUA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/src/libraries/luau/VM/include")
set(LUA_LIBRARIES luau)

# 3. Prevent LÖVE from trying to find LuaJIT's FFI (which Luau doesn't have)
add_definitions(-DLUA_5_1)
