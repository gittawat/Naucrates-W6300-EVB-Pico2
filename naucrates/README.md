# naucrates — Firmware Source Tree

```
naucrates/
├── CMakeLists.txt
├── utilities/              Header-only infrastructure
│   ├── rtt_logger.hpp      RTTLogger — SWD logging
│   └── triple_buffer.hpp   TripleBuffer — wait-free inter-core exchange
├── features/               Feature modules
│   └── blinky/             Blinky — GPIO LED toggle helper
├── drivers/wiznet/         W6300 Ethernet controller C++ wrapper
│   ├── w6300_driver.hpp
│   └── w6300_driver.cpp
├── main/                   Firmware entry point and board-level config
│   ├── main.cpp
│   └── firmware_config.hpp
└── tests/                  On-target tests (triple_buffer_mcu_test)
```

`ignore_this/` holds the previous module-framework experiment
(`ModuleConcept`, `StaticModuleRunner`) and is intentionally gitignored.

---

## `utilities/`

Header-only infrastructure shared by all features and drivers.

**CMake target:** `naucrates_utilities` (INTERFACE)

Pulls in vendor and SDK dependencies used by every naucrates component:
- `etl` — Embedded Template Library
- `rtt` — SEGGER RTT logging over SWD
- `pico_stdlib` — Raspberry Pi Pico SDK standard library
- `hardware_irq`, `hardware_gpio` — Pico SDK hardware abstraction

Adds `${CMAKE_SOURCE_DIR}` to the include path so all naucrates headers are
reachable via `#include "naucrates/utilities/..."`.

### Files

| File | Purpose |
|------|---------|
| `rtt_logger.hpp` | RTT-based `write()`, `print()` |
| `triple_buffer.hpp` | Wait-free triple-buffer for inter-core communication |

---

## `features/`

Feature modules built into the firmware.

**CMake target:** `naucrates_features` (STATIC)

Links `naucrates_common` → `naucrates_utilities`.

### Features

| Directory | Class | Description |
|-----------|-------|-------------|
| `blinky/` | `Blinky` | GPIO LED toggle helper — `init()` + `toggle()`, caller owns timing |

---

## `drivers/wiznet/`

C++ wrapper around the vendor `wiz6300-lib` C library.

**CMake target:** `naucrates_wiznet` (STATIC)

Compiles `w6300_driver.cpp` against `wiz6300` (vendor STATIC library) and
links `naucrates_common` → `naucrates_utilities`. Link usage is `PUBLIC` so
consumers inherit `wiz6300`'s include paths and compile definitions
(`_WIZCHIP_=W6300`, board defines).

### Files

| File | Purpose |
|------|---------|
| `w6300_driver.hpp` | `W6300Driver` — init, network config, UDP socket API, interrupt helpers |
| `w6300_driver.cpp` | Implementation against the wiz6300-lib C API |

---

## `main/`

Firmware entry point and board-level configuration.

**CMake target:** `naucrates` (EXECUTABLE)

Links `naucrates_common`, `pico_multicore`. Currently a minimal stub that
initializes the RTT logger. Feature and driver targets are commented out in
`main/CMakeLists.txt` until wired into the entry point.

### Files

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point, main loop |
| `firmware_config.hpp` | `naucrates::config` — servo frequency, GPIO pin assignments, compile-time pin uniqueness check |

---

## `tests/`

On-target test executables flashed to the board via SWD.

**CMake target:** `triple_buffer_mcu_test` (EXECUTABLE)

Links `naucrates_common`, `pico_multicore`. Exercises the
`TripleBuffer` across the two RP2350 cores and reports PASS/FAIL over RTT.

---

## Build Chain

```
etl, rtt, wiz6300 (STATIC)
        │
        ▼
naucrates_utilities (INTERFACE)     — etl, rtt, pico_stdlib, hardware deps
        │
        ▼
naucrates_common (INTERFACE)        — -Wall -Wextra -Wno-unused-parameter
        │
   ┌────┴─────┬──────────┐
   ▼          ▼          ▼
features    drivers     tests
(STATIC)    /wiznet     (EXECUTABLE)
            (STATIC)
        │
        ▼
naucrates (EXECUTABLE)              — main.cpp + pico_multicore
```

## Adding Source Files

1. Drop `.hpp`/`.cpp` into the appropriate subdirectory.
2. Add minimal CMake wiring:
   - For a new feature: add the `.cpp` to `features/CMakeLists.txt`:
     ```cmake
     target_sources(naucrates_features PRIVATE <name>/<name>_feature.cpp)
     ```
   - For the W6300 driver wrapper: add source files to
     `drivers/wiznet/CMakeLists.txt`.
3. Instantiate and register in `main/main.cpp`, then link the target in
   `main/CMakeLists.txt` (uncomment `naucrates_features` / `naucrates_wiznet`).
4. Add any new config structs to `main/firmware_config.hpp`.
