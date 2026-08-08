# naucrates

Modular embedded firmware for the **W6300-EVB-Pico2** (RP2350), built with
modern C++23 and the Embedded Template Library (ETL).

## Dependencies

| Component | Location | Purpose |
|-----------|----------|---------|
| Pico SDK | `sdk/pico-sdk` (submodule) | RP2350 hardware abstraction |
| ETL 20.48.0 | `library/etl` (submodule) | No-heap containers |
| SEGGER RTT | `library/RTT` (submodule) | SWD logging output |
| wiz6300-lib | `library/wiz6300-lib` | W6300 Ethernet chip driver (C) |

## Project Structure

```
naucrates/
├── utilities/       Header-only infrastructure (naucrates_utilities)
│   ├── rtt_logger.hpp   RTTLogger — SWD logging
│   └── triple_buffer.hpp  TripleBuffer — wait-free inter-core exchange
├── features/        Feature modules (naucrates_features)
│   └── blinky/           Blinky — GPIO LED toggle helper
├── drivers/wiznet/  W6300Driver C++ wrapper (naucrates_wiznet)
│   ├── w6300_driver.hpp
│   └── w6300_driver.cpp
├── main/            Firmware entry point and board-level configuration
│   ├── main.cpp
│   └── firmware_config.hpp
└── tests/           On-target tests (triple_buffer_mcu_test)
```

`naucrates/ignore_this/` holds the previous module-framework experiment
(`ModuleConcept`, `StaticModuleRunner`) and is intentionally gitignored.

## Building

Configure and build using CMake presets:

```bash
cmake --preset debug              # configure (build/debug/)
cmake --build --preset debug      # build
cmake --build --preset debug-clean # clean + build
```

For release builds, substitute `release` / `release-clean`.

## Flashing

Flash the firmware via probe-rs over SWD:

```bash
probe-rs download --chip RP235x --protocol swd build/debug/naucrates.elf \
  && probe-rs reset --chip RP235x --protocol swd
```

For the release build, use `build/release/naucrates.elf`.

To flash and run the triple-buffer test:

```bash
probe-rs download --chip RP235x --protocol swd build/debug/naucrates/tests/triple_buffer_mcu_test.elf \
  && probe-rs reset --chip RP235x --protocol swd
```

## Adding a Feature

1. Create `naucrates/features/<name>/<name>.hpp` + `.cpp`.
2. Add the `.cpp` to `naucrates/features/CMakeLists.txt`:
   ```cmake
   target_sources(naucrates_features PRIVATE
       blinky/blinky.cpp
       <name>/<name>.cpp
   )
   ```
3. Instantiate and use it in `naucrates/main/main.cpp`.
4. Add any new config structs to `naucrates/main/firmware_config.hpp`.


## SDK and Library doc gen
```bash
  cd build_doc/pico-sdk-doc
  cmake ../../sdk/pico-sdk -DPICO_BUILD_DOCS=1 -DPICO_PLATFORM=rp2350 -DPICO_NO_PICOTOOL=TRUE -G "Ninja"

  cd build_doc/etl-docs
  doxygen ../../library/etl/Doxyfile
```

## License

TBD
