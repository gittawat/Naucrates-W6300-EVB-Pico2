# naucrates — Firmware Source Tree

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

Header-only infrastructure shared by all modules and drivers. No `.cpp` files.

| File | Purpose |
|------|---------|
| `interrupt_handlers.hpp` | Compile-time GPIO interrupt binding (`attach_gpio_interrupt`). C++20 concept-checked, zero-overhead stateless lambda ISR generator. |
| `rtt_logger.hpp` | SEGGER RTT-based logging over SWD (no UART needed). Provides `write()`, `print()`, and `hex_dump()`. |

**CMake target:** `naucrates_platform` (INTERFACE)
- Pulls in `etl`, `rtt`, `pico_stdlib`, `hardware_irq`, `hardware_gpio`
- Adds `${CMAKE_SOURCE_DIR}` to include path so all naucrates headers are reachable via `#include "naucrates/..."`

---

## `modules/`

Module framework and concrete module implementations.

| File | Purpose |
|------|---------|
| `module_concepts.hpp` | C++20 concepts: `ModuleConcept` (requires `void update()`), `ConfigurableModule`, `InterruptHandlingModule` |
| `shared_data.hpp` | `SharedData` struct — RX/TX buffers used for inter-module communication (LinuxCNC Remora-style) |
| `static_module_runner.hpp` | `StaticModuleRunner<Modules...>` — variadic fold-expression dispatcher with hardware-timer rate limiting |

### `udp_echo/`

| File | Purpose |
|------|---------|
| `udp_echo_module.hpp` | `UdpEchoModule` class — interrupt-driven UDP echo with statistics and heartbeat LED |
| `udp_echo_module.cpp` | RX/TX packet processing, deferred interrupt handling via atomic flags |

**CMake target:** `naucrates_modules` (INTERFACE in skeleton, STATIC when sources are added)
- Links `naucrates_common` (→ `naucrates_platform`)

---

## `drivers/wiznet/`

C++ wrapper around the vendor `wiz6300-lib` C library.

| File | Purpose |
|------|---------|
| `w6300_driver.hpp` | `W6300Driver` class — init, network config, UDP socket open/close, recvfrom/sendto |
| `w6300_driver.cpp` | SPI initialization, socket interrupt mask management, RX ring buffer polling |

**CMake target:** `naucrates_wiznet` (STATIC)
- Links `wiz6300` (vendor C library) and `naucrates_common` (→ `naucrates_platform`)

---

## `main/`

Firmware entry point and board-level configuration.

| File | Purpose |
|------|---------|
| `main.cpp` | Static module instances, ISR definitions, `StaticModuleRunner` main loop |
| `firmware_config.hpp` | `naucrates::config` namespace — servo frequency, UDP echo parameters, network info |

**CMake target:** `naucrates` (executable)
- Links `naucrates_wiznet`, `naucrates_modules`, `naucrates_platform`, `pico_multicore`

---

## Build Chain

```
naucrates_platform (INTERFACE)     — etl, rtt, pico_stdlib, hardware deps
        ↑
naucrates_common (INTERFACE)       — adds -Wall -Wextra for all naucrates code
        ↑
   ┌────┴────┐
   │         │
modules    drivers/wiznet
(INTERFACE)  (STATIC when sources exist)
   │         │
   └────┬────┘
        ↓
naucrates (EXECUTABLE)             — main.cpp + module sources
```

## Adding a New Module

1. Create `naucrates/modules/<name>/<name>.hpp` + `.cpp`
2. Add source to `naucrates/main/CMakeLists.txt` in the `add_executable(naucrates ...)` block
3. Register the module instance in `main.cpp`:
   ```cpp
   static MyModule mod(config::MY_MODULE);
   irq::attach_gpio_interrupt<mod, MY_PIN, GPIO_IRQ_EDGE_RISE>();
   mod.configure();
   static StaticModuleRunner runner(config::SERVO_FREQ_HZ, echo, mod);
   ```
