# naucrates — Firmware Source Tree

> **Skeleton branch.** This branch contains the CMake build infrastructure and
> library patches only. Source files (`.cpp`, `.hpp`) live on feature branches
> and drop into the directories below without CMake changes — all targets and
> link dependencies are pre-wired.

```
naucrates/
├── CMakeLists.txt
├── platform/              Header-only utilities and hardware abstractions
├── modules/               Module framework and module implementations
│   └── udp_echo/          UDP echo module
├── drivers/wiznet/        W6300 Ethernet controller C++ driver
└── main/                  Firmware entry point and board-level configuration
```

---

## `platform/`

Header-only infrastructure shared by all modules and drivers.

**CMake target:** `naucrates_platform` (INTERFACE)

Pulls in vendor and SDK dependencies used by every naucrates component:
- `etl` — Embedded Template Library (containers, atomics)
- `rtt` — SEGGER RTT logging over SWD
- `pico_stdlib` — Raspberry Pi Pico SDK standard library
- `hardware_irq`, `hardware_gpio` — Pico SDK hardware abstraction

Adds `${CMAKE_SOURCE_DIR}` to the include path so all naucrates headers are
reachable via `#include "naucrates/platform/..."`.

Currently idle — no consumers have source files yet. Dependencies activate
when `.cpp` files are added to `modules/` or `drivers/`.

### Expected source files (from feature branches)

| File | Purpose |
|------|---------|
| `interrupt_handlers.hpp` | Compile-time GPIO interrupt binding |
| `rtt_logger.hpp` | RTT-based `write()`, `print()`, `hex_dump()` |

---

## `modules/`

Module framework and concrete module implementations.

**CMake target:** `naucrates_modules` (INTERFACE)

Links `naucrates_common` → `naucrates_platform`. When source files are added,
change to `STATIC` and add `target_sources(...)`.

### Expected source files (from feature branches)

| File | Purpose |
|------|---------|
| `module_concepts.hpp` | C++20 concepts: `ModuleConcept`, `ConfigurableModule`, `InterruptHandlingModule` |
| `shared_data.hpp` | `SharedData` — RX/TX buffers for inter-module communication |
| `static_module_runner.hpp` | `StaticModuleRunner<Modules...>` — fold-expression dispatcher |
| `udp_echo/udp_echo_module.hpp` | `UdpEchoModule` — interrupt-driven UDP echo |
| `udp_echo/udp_echo_module.cpp` | RX/TX processing, deferred interrupt handling |

---

## `drivers/wiznet/`

C++ wrapper around the vendor `wiz6300-lib` C library.

**CMake target:** `naucrates_wiznet` (INTERFACE)

Links `wiz6300` (vendor) and `naucrates_common` → `naucrates_platform`. When
source files are added, change to `STATIC` and add `target_sources(...)`.

### Expected source files (from feature branches)

| File | Purpose |
|------|---------|
| `w6300_driver.hpp` | `W6300Driver` — init, network config, UDP socket operations |
| `w6300_driver.cpp` | SPI init, interrupt mask management, RX polling |

---

## `main/`

Firmware entry point and board-level configuration.

**CMake target:** `naucrates` (EXECUTABLE) — template commented out in `CMakeLists.txt`.

Links `naucrates_wiznet`, `naucrates_modules`, `naucrates_common`, `pico_multicore`.

### Expected source files (from feature branches)

| File | Purpose |
|------|---------|
| `main.cpp` | Static module instances, ISR definitions, main loop |
| `firmware_config.hpp` | `naucrates::config` — servo frequency, network info |

---

## Build Chain

```
naucrates_platform (INTERFACE)     — etl, rtt, pico_stdlib, hardware deps
        ↑
naucrates_common (INTERFACE)       — -Wall -Wextra -Wno-unused-parameter
        ↑
   ┌────┴────┐
   │         │
modules    drivers/wiznet
(INTERFACE)  (INTERFACE)
   │         │
   └────┬────┘
        ↓
naucrates (EXECUTABLE)             — main.cpp + module sources
```

## Adding Source Files

1. Drop `.hpp`/`.cpp` into the appropriate subdirectory (see "Expected source files" above).
2. Uncomment the `add_executable` block in `main/CMakeLists.txt`.
3. No other CMake changes are needed — all targets and link dependencies are pre-wired.
