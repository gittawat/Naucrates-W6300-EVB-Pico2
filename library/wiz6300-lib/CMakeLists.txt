# W6300 Ethernet Library
# Combines WIZnet ioLibrary_Driver (chip-agnostic socket/wizchip) and
# Pico port layer (QSPI via PIO, GPIO IRQ) for the W6300-EVB-Pico2 board.
#
# Usage from parent project:
#   set(PICO_PLATFORM rp2350)
#   add_subdirectory(path/to/wiz6300-lib)
#   target_link_libraries(your_app wiz6300)

cmake_minimum_required(VERSION 3.13)

# ---------------------------------------------------------------------------
# Static library target
# ---------------------------------------------------------------------------
add_library(wiz6300 STATIC)

target_sources(wiz6300 PRIVATE
    # ioLibrary core
    ioLibrary/socket.c
    ioLibrary/wizchip_conf.c
    # W6300 HAL
    ioLibrary/W6300/w6300.c
    # Port layer (Pico-specific)
    port/wizchip_spi.c
    port/wizchip_qspi_pio.c
    port/wizchip_gpio_irq.c
)

# ---------------------------------------------------------------------------
# PIO header generation (must come after target creation)
# ---------------------------------------------------------------------------
pico_generate_pio_header(wiz6300
    ${CMAKE_CURRENT_SOURCE_DIR}/port/wizchip_qspi_pio.pio
)

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
target_include_directories(wiz6300 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ioLibrary          # socket.h, wizchip_conf.h, Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ioLibrary/W6300    # w6300.h
    ${CMAKE_CURRENT_SOURCE_DIR}/port               # all port headers
)

# ---------------------------------------------------------------------------
# Compile definitions (PUBLIC — consumers inherit)
# ---------------------------------------------------------------------------
target_compile_definitions(wiz6300 PUBLIC
    _WIZCHIP_=W6300
    DEVICE_BOARD_NAME=W6300_EVB_PICO2
    _WIZCHIP_QSPI_MODE_=QSPI_SINGLE_MODE
    PICO_USE_FASTEST_SUPPORTED_CLOCK=1
)

# ---------------------------------------------------------------------------
# SDK dependencies
# ---------------------------------------------------------------------------
target_link_libraries(wiz6300 PUBLIC
    pico_stdlib
    hardware_pio
    hardware_spi
    hardware_dma
    hardware_clocks
    hardware_gpio
    hardware_irq
)
