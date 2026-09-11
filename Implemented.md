# JOCKY Framework - Implementation Status

This document tracks the features from the original `Vision.md` that have been successfully implemented in the `v0.1` JOCKY compiler.

## Core Language & Compiler (Phase 1)
- [x] **Proprietary Language Syntax**: Basic lexer, parser, and semantic analyzer implemented for `.jky` files.
- [x] **Transpilation Pipeline**: Successfully implemented the "Option A" architecture from the Vision document: JOCKY source -> C code -> MSVC -> Executable payload.
- [x] **Zero-Disk Artifact Destruction**: The compiler simulates an in-memory STDIN pipe to the MSVC backend (`cl.exe`) and instantly shreds the intermediate `_jky_stealth_pipe.c` and `.obj` files once the polymorphic binary (`agent_v01.exe`) is emitted.
- [x] **Basic Operations**: Support for `fn main() -> void`, `while` loops, integer declarations, operators, and standard I/O via `log.info()`.

## Forensic Extraction Capabilities
- [x] **Authentication Log Extraction**: Added `forensics.get_auth_logs(timeframe_hours)`.
- [x] **Native Windows API/PowerShell Bypassing**: Instead of relying on loud C APIs or requiring heavy XML parsers, the runtime leverages native PowerShell (`Get-WinEvent`) behind the scenes to stealthily query the Security Event Log (4624, 4625) and extract exact tables containing:
  - Timestamp (`TimeCreated`)
  - Event ID (`Id`)
  - Username (`User`)
  - Source Network IP Address (`IP`)
- [x] **Data Persistence & Formatting**: Compiler automatically intercepts the forensics command and emits C code to print the structured table to the terminal and covertly save it to `auth_logs.txt` in the deployment directory.

## Build System & Organization
- [x] **Automated Build Wrapper**: Created robust `custom_build.bat` that seamlessly locates the MSVC `vcvars64.bat` environment and compiles the compiler into a clean `bin/` directory.
- [x] **Artifact Separation**: Updated the code generator to neatly redirect all intermediate runtime compilation `.obj` and `.pdb` files into the `temp/` folder, preventing root directory clutter.
- [x] **Git Ignored Artifacts**: Configured `.gitignore` to drop all `.exe`, `.obj`, `.pdb`, `temp/`, and runtime logs (`auth_logs.txt`), ensuring operational opsec and clean commits.

## Next Steps (Planned for v0.2+)
- [ ] Implement `package` and `const` keywords (as envisioned in `examples/hello.jky`).
- [ ] Implement `jky_fs.c` metadata and hashing functions (`jockey_scan`, `jockey_extract`).
- [ ] Add strings, bytes, and multi-return tuples to the core parser.
