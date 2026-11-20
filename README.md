# Zen_gdb

A specialized x86_64 dynamic malware analysis debugger and execution monitor designed for analyzing stripped, packed, and hostile Windows PE executables.

---

## Architecture Overview

```
Zen_gdb/
├── CMakeLists.txt             # Modern CMake build configuration (C++20)
├── .gitignore                 # Artifact & build filter
├── src/
│   ├── main.cpp               # CLI entrypoint & argument parsing
│   ├── debugger.hpp           # Core Debugger engine header & state definitions
│   ├── debugger.cpp           # Win32 Debug Event Loop & process lifecycle manager
│   ├── registers.hpp          # Thread context & x86_64 register inspection
│   └── registers.cpp          # Register formatting, flag decoder, and Trap Flag controls
└── samples/
    └── dummy_target.cpp       # Synthetic test executable for debugger validation
```

---

## Core Capabilities (Current Milestone)

- **Target Process Control:**
  - Spawning isolated child targets via `CreateProcessW` with `DEBUG_ONLY_THIS_PROCESS`.
  - Attaching dynamically to running processes via `DebugActiveProcess`.
- **Win32 Event Loop:**
  - Non-blocking dispatch of process creation, thread lifecycles, DLL loading/unloading, and exceptions using `WaitForDebugEvent`.
  - Mandatory resource management (clean closing of `hFile` file descriptors).
- **Register Context & State Management:**
  - 64-bit general-purpose register extraction (`RAX`–`R15`, `RIP`, `RSP`, `RBP`, `RFLAGS`) via `GetThreadContext`.
  - Individual RFLAGS condition decoding (`CF`, `PF`, `AF`, `ZF`, `SF`, `TF`, `IF`, `DF`, `OF`).
  - Instruction-level single stepping via CPU Trap Flag (`TF`) modification.
- **Interactive Debug Shell:**
  - Breakpoint and exception halt prompts with interactive commands:
    - `c` / `continue`: Resume execution.
    - `s` / `step`: Single-step instruction using CPU Trap Flag.
    - `r` / `regs`: Display full register state and active CPU flags.
    - `q` / `quit`: Terminate debuggee and exit cleanly.

---

## Update Timeline

### v0.1.0 - Core Engine & Win32 Debug Subsystem
- **Process Lifecycle Management:** Process launching with `DEBUG_ONLY_THIS_PROCESS` and PID attachment via `DebugActiveProcess`.
- **Event Dispatch Loop:** Win32 debug event handling for process, thread, module loading, and exception trapping.
- **Context & Registers:** Full x86_64 general-purpose register extraction (`RAX`–`R15`, `RIP`, `RSP`, `RBP`) and bitmask decoding for CPU status flags (`CF`, `ZF`, `SF`, `OF`, `TF`, etc.).
- **Execution Flow Control:** Single-step instruction stepping via CPU Trap Flag (`TF`) manipulation.
- **Interactive Shell:** Responsive breakpoint console supporting execution resume, stepping, register dumps, and clean termination.
- **Test Harness:** Included synthetic target binary (`dummy_target`) to validate anti-debugging detection and memory loop execution.

---

## Building from Source

### Prerequisites
* Windows 10 / 11 (x86_64)
* CMake 3.20+
* Visual Studio 2022 C++ Build Tools (MSVC) or Clang-cl

### Build Instructions
```powershell
# Generate build configuration
cmake -B build -S .

# Build Zen_gdb and test target
cmake --build build --config Release
```

### Running
```powershell
# Launch target under debugger
.\build\bin\Release\Zen_gdb.exe .\build\bin\Release\dummy_target.exe

# Or attach to an existing process
.\build\bin\Release\Zen_gdb.exe -p <PID>
```
