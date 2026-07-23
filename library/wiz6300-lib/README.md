# W6300 Ethernet Library

Vendored subset of WIZnet ioLibrary + Pico port layer for the W6300-EVB-Pico2 board
(RP2350 + W6300, single-line SPI via PIO).

## Source origin

| Directory | Source repo | Path within upstream |
|---|---|---|
| `ioLibrary/socket.{c,h}` | [Wiznet/ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver) | `Ethernet/socket.{c,h}` |
| `ioLibrary/wizchip_conf.{c,h}` | [Wiznet/ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver) | `Ethernet/wizchip_conf.{c,h}` |
| `ioLibrary/Application.h` | [Wiznet/ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver) | `Application/Application.h` |
| `ioLibrary/W6300/w6300.{c,h}` | [Wiznet/ioLibrary_Driver](https://github.com/Wiznet/ioLibrary_Driver) | `Ethernet/W6300/w6300.{c,h}` |
| `port/wizchip_spi.{c,h}` | [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) | `port/ioLibrary_Driver/src/wizchip_spi.c`, `.../inc/wizchip_spi.h` |
| `port/wizchip_qspi_pio.{c,h,pio}` | [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) | `port/ioLibrary_Driver/src/wizchip_qspi_pio.{c,pio}`, `.../inc/wizchip_qspi_pio.h` |
| `port/wizchip_gpio_irq.{c,h}` | [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) | `port/ioLibrary_Driver/src/wizchip_gpio_irq.c`, `.../inc/wizchip_gpio_irq.h` |

> **Note:** `wizchip_gpio_irq.{c,h}` are kept on disk for reference but **not
> compiled**. GPIO interrupt setup is handled by the consumer's app-layer
> interrupt handlers (see `naucrates/platform/interrupt_handlers.hpp`).
| `port/board_list.h` | [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) | `port/board_list.h` |
| `port/port_common.h` | [WIZnet-ioNIC/WIZnet-PICO-C](https://github.com/WIZnet-ioNIC/WIZnet-PICO-C) | `port/port_common.h` |

## Modifications from upstream

Three changes from the original sources:

| File | Change | Reason |
|---|---|---|
| `ioLibrary/wizchip_conf.h` | `#include "../Application/Application.h"` → `#include "Application.h"` | Application.h moved to same directory to flatten layout |
| `port/wizchip_spi.c` | PHY link check: `while (temp == PHY_LINK_OFF);` → one-shot status report | Upstream hangs forever with no Ethernet cable plugged in; we report link status and continue |
| `CMakeLists.txt` | Removed `port/wizchip_gpio_irq.c` from `target_sources`; made includes `SYSTEM` | Interrupt setup moved to app-layer interrupt handlers (see project code); `SYSTEM` suppresses third-party C header warnings in consumer code |

## Compile definitions

Set by `CMakeLists.txt` and propagated to consumer targets (`PUBLIC`):

| Define | Value | Purpose |
|---|---|---|
| `_WIZCHIP_` | `W6300` | Selects W6300 HAL (w6300.c/h), 8 sockets, 4KB/socket |
| `DEVICE_BOARD_NAME` | `W6300_EVB_PICO2` | Selects pin map (GPIO 15-22) and enables `USE_PIO` |
| `_WIZCHIP_QSPI_MODE_` | `QSPI_SINGLE_MODE` | Single-line SPI (SCK+MOSI+MISO, frees GPIO20,21) |
| `PICO_USE_FASTEST_SUPPORTED_CLOCK` | `1` | Max CPU clock for fastest SPI throughput |

## API surface

The library provides a Berkeley-socket-like API for UDP/TCP. Key functions:

```c
#include "socket.h"        // socket, recvfrom, sendto, close, getsockopt
#include "wizchip_conf.h"  // wiz_NetInfo, ctlwizchip, ctlnetwork
#include "wizchip_spi.h"   // wizchip_spi_initialize, wizchip_initialize, network_initialize
```

See the W6300 datasheet and ioLibrary documentation for full API details.

## Build

```cmake
set(PICO_PLATFORM rp2350)
add_subdirectory(path/to/wiz6300-lib)
target_link_libraries(your_app wiz6300)
```

The `wiz6300` target is a `STATIC` library that links `pico_stdlib`, `hardware_pio`, `hardware_spi`,
`hardware_dma`, `hardware_clocks`, `hardware_gpio`, and `hardware_irq`.
