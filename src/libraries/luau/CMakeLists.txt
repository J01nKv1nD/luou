# This file tells CMake how to build the Luau parts
project(luau)

# Gather all the source files
file(GLOB_RECURSE LUAU_SOURCES
    "VM/src/*.cpp"
    "Compiler/src/*.cpp"
    "Ast/src/*.cpp"
    "Common/src/*.cpp"
)

# This vendored Luau snapshot doesn't include CodeGen/BytecodeBuilder headers.
list(FILTER LUAU_SOURCES EXCLUDE REGEX "Compiler/src/(Compiler|Types)\\.cpp$")

# Create a static library called 'luau'
add_library(luau STATIC ${LUAU_SOURCES})

# Tell other parts of the engine where the headers are
target_include_directories(luau PUBLIC
    "VM/include"
    "Compiler/include"
    "Ast/include"
    "Common/include"
)
