#ifndef NAUCRATES_FIRMWARE_CONFIG_HPP
#define NAUCRATES_FIRMWARE_CONFIG_HPP

#include <cstdint>
#include <cstddef>
#include "wizchip_conf.h"

#include "naucrates/modules/udp_echo/udp_echo_module.hpp"

namespace naucrates::config
{

inline constexpr int32_t SERVO_FREQ_HZ = 1000;

inline constexpr UdpEchoConfig UDP_ECHO = {
    .socket_id   = 0,
    .port        = 27181,
    .rx_buf_size = 2048,
    .history_cap = 10,
    .led_pin     = 25,
    .net_info    = {
        .mac    = {0x00, 0x08, 0xDC, 0x63, 0x00, 0x10},
        .ip     = {10, 10, 10, 10},
        .sn     = {255, 255, 255, 0},
        .gw     = {10, 10, 10, 1},
        .lla    = {},
        .gua    = {},
        .sn6    = {},
        .gw6    = {},
        .dns    = {8, 8, 8, 8},
        .dns6   = {},
        .ipmode = NETINFO_STATIC_V4,
        .dhcp   = NETINFO_STATIC,
    },
};

} // namespace naucrates::config

#endif // NAUCRATES_FIRMWARE_CONFIG_HPP
