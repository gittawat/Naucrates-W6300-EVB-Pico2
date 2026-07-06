---

# Pico-SDK RP2350 Project starter point

---

## Building and Flashing

### Configuring

Choose a configuration preset:
```bash
# Configure debug or release
cmake --preset debug
cmake --preset release
```

### Building & Flashing (using Presets)

After configuring, you can use build presets to build, clean build, or flash:
```bash
# Build (standard)
cmake --build --preset debug
cmake --build --preset release

# Clean & Build (proper clean build)
cmake --build --preset debug-clean
cmake --build --preset release-clean

# Build & Flash
cmake --build --preset debug-flash
cmake --build --preset release-flash
```

### Traditional Building and Flashing

Alternatively, you can build directly using directories:
```bash
# Build
cmake --build build/{debug|release} 

# Build and Flash
cmake --build build/{debug|release} --target flash

# Clean build (rebuild)
cmake --build build/{debug|release} --target clean && cmake --build build/{debug|release}

# Full clean (remove build directory)
rm -rf build/
```


## To-Do

* [x] **Language Server:** Setup a proper `.clangd` file for setup embedded system development
* [x] **Build Automation:** Separate the structure into proper `debug` and `release` directories.
