# naucrates

Modular embedded firmware for the **W6300-EVB-Pico2** (RP2350), built with
modern C++23 and the Embedded Template Library (ETL). Designed as a
Remora-style module framework for real-time control.

Uses **C++20 concepts** for module typing and **C++17 fold expressions** for
compile-time module dispatch — zero runtime overhead (no vtables, no function
pointers, no heap allocations).

## Dependencies

| Component | Location | Purpose |
|-----------|----------|---------|
| Pico SDK | `sdk/pico-sdk` (submodule) | RP2350 hardware abstraction |
| ETL 20.48.0 | `library/etl` (submodule) | No-heap containers, atomics |
| SEGGER RTT | `library/RTT` (submodule) | SWD logging output |
| wiz6300-lib | `library/wiz6300-lib` | W6300 Ethernet chip driver (C) |

## Project Structure

```
naucrates/
├── platform/          Header-only utilities (interrupt_handlers, RTTLogger)
├── modules/           Module framework (concepts, static_runner, SharedData)
│   └── udp_echo/      UDP echo module
├── drivers/wiznet/    W6300Driver C++ wrapper
└── main/              Firmware orchestrator, config, ISR handlers
```

## Architecture

### Modules (C++20 Concepts)

Modules implement `void update()` and satisfy `ModuleConcept`. Optionally
implement `void configure()` (`ConfigurableModule`) and
`void handle_interrupt()` (`InterruptHandlingModule`).

No base class inheritance. The compiler checks constraints at compile time.

### Interrupts (Direct Compile-Time Binding)

ISRs are `extern "C"` functions defined in `main.cpp` that directly call the
static module instance. Zero indirection — the compiler resolves the address
at link time and can inline the handler.

Concept checking (`irq::check_interrupt_handler<T>()`) validates at compile
time that the bound module implements `void handle_interrupt()`.

### Module Runner (Fold Expression)

`StaticModuleRunner<Modules...>` uses `std::tuple` and C++17 fold expressions
to unroll module updates into sequential direct calls — no list nodes, no
iterator loops, no virtual dispatch.

## Building and Flashing

### Configure

```bash
cmake --preset debug
cmake --preset release
```

### Build

```bash
cmake --build --preset debug
cmake --build --preset release
```

### Clean & Build

```bash
cmake --build --preset debug-clean
cmake --build --preset release-clean
```

### Flash (via probe-rs SWD)

```bash
cmake --build --preset debug-flash
cmake --build --preset debug-clean-flash
```

## Adding a Module

1. Create `naucrates/modules/<name>/<name>.hpp` + `.cpp`
2. Implement `void update()` (required), optionally `void configure()` and
   `void handle_interrupt()`
3. Add to `naucrates/main/CMakeLists.txt`:
   ```cmake
   ../modules/<name>/<name>.cpp
   ```
4. Register in `naucrates/main/main.cpp`:
   ```cpp
   static MyModule mod(config::MY_MODULE);
   mod.configure();
   static StaticModuleRunner runner(config::SERVO_FREQ_HZ, echo, mod);
   ```

## License

TBD
