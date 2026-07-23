# naucrates

Modular embedded firmware for the **W6300-EVB-Pico2** (RP2350), built with
modern C++23 and the Embedded Template Library (ETL). Designed as a
Remora-style module framework for real-time control.

## Dependencies

| Component | Location | Purpose |
|-----------|----------|---------|
| Pico SDK | `sdk/pico-sdk` (submodule) | RP2350 hardware abstraction |
| ETL 20.48.0 | `library/etl` (submodule) | No-heap containers, delegates, atomics |
| SEGGER RTT | `library/RTT` (submodule) | SWD logging output |
| wiz6300-lib | `library/wiz6300-lib` | W6300 Ethernet chip driver (C) |

## Project Structure

```
naucrates/
├── platform/       Header-only utilities (InterruptManager, RTTLogger)
├── modules/        Module framework (Module base, ModuleRunner, SharedData)
│   └── udp_echo/   UDP echo module (mockup)
├── drivers/wiznet/ W6300Driver C++ wrapper
└── main/           Firmware orchestrator, config, ISR trampolines
```

## Building and Flashing

### Configure

```bash
cmake --preset debug
cmake --preset release
```

### Build

```bash
# Standard build (Ninja)
cmake --build --preset debug
cmake --build --preset release

# Clean & build
cmake --build --preset debug-clean
cmake --build --preset release-clean
```

### Flash (via probe-rs SWD)

```bash
# Build & flash
cmake --build --preset debug-flash

# Clean, build & flash
cmake --build --preset debug-clean-flash
```

## Adding a Module

1. Create `naucrates/modules/<name>/<name>.hpp` + `.cpp` (inherit `Module`)
2. Add one line to `naucrates/main/CMakeLists.txt`:
   ```cmake
   ../modules/<name>/<name>.cpp
   ```
3. Register in `naucrates/main/main.cpp`:
   ```cpp
   static MyModule mod(config::MY_MODULE);
   mod.configure();
   runner.register_module(mod);
   ```

## License

TBD
