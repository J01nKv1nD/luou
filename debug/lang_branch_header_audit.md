# Lang Branch Header Audit (workspace snapshot)

Date: 2026-04-28 (UTC)
Branch checked: `work`

## Scope requested
1. Luau VM Headers (Core Logic)
2. Luau Compiler Headers ("Brains")
3. Custom Bridge Header (`src/common/lua.hpp`)
4. LÖVE Module Headers

## Result
- `lang` branch is not present in this checkout.
- Only local branch is `work`.
- `src/common/lua.hpp` is not present.
- No LÖVE module headers detected (`love/*.h`, `LOVE`, `lua.hpp` bridge paths).

## Header availability in current tree

### Luau VM headers
Present under `VM/include`:
- `lua.h`
- `lualib.h`
- `luacode.h`
- `luaconf.h`
- `luacodegen.h`

### Luau Compiler headers
Present under `Compiler/include/Luau`:
- `Compiler.h`
- `BuiltinFolding.h`
- `BytecodeBuilder.h`
- `CostModel.h`
- `TableShape.h`

### Custom bridge header
- Missing: `src/common/lua.hpp`

### LÖVE headers
- Missing in this checkout.

## Include model note
This tree uses split include roots in build system (Makefile/CMake):
- VM include root: `-IVM/include`
- Compiler include root: `-ICompiler/include`
- Plus `Common/include`, `Ast/include`, `Bytecode/include`, etc.

That matches "multiple headers" model. Cross-component code should include by module root (e.g. `#include "lua.h"` for VM C API and `#include "Luau/Compiler.h"` for compiler API) and ensure target-specific include paths stay explicit.

## Commands used
- `git branch -a`
- `rg --files | rg 'src/common/lua.hpp|lua.hpp|LOVE|love|engine|lang'`
- `rg --files VM/include Compiler/include`
