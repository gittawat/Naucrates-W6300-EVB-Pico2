#ifndef NAUCRATES_FIRMWARE_CONFIG_HPP
#define NAUCRATES_FIRMWARE_CONFIG_HPP

#include <cstddef>
#include <cstdint>

namespace naucrates::config
{

inline constexpr int32_t SERVO_FREQ_HZ = 1000;

struct BlinkyConfig
{
    uint8_t led_pin        = 25;
    uint32_t toggle_freq_hz = 2;
};

struct W6300Config
{
    uint8_t pin_miso = 19;
    uint8_t pin_cs   = 16;
    uint8_t pin_sck  = 17;
    uint8_t pin_mosi = 18;
    uint8_t pin_rst  = 22;
    uint8_t pin_int  = 15;
};

inline constexpr BlinkyConfig BLINKY{};
inline constexpr W6300Config W6300{};

template <uint8_t... Pins>
consteval bool are_pins_unique()
{
    constexpr uint8_t pin_array[] = { Pins... };
    for (size_t i = 0; i < sizeof...(Pins); ++i)
    {
        for (size_t j = i + 1; j < sizeof...(Pins); ++j)
        {
            if (pin_array[i] == pin_array[j]) return false;
        }
    }
    return true;
}

static_assert(are_pins_unique<
    BLINKY.led_pin,
    W6300.pin_miso,
    W6300.pin_cs,
    W6300.pin_sck,
    W6300.pin_mosi,
    W6300.pin_rst,
    W6300.pin_int
>(), "FATAL BUILD ERROR: Duplicate GPIO pin assignment detected in firmware_config.hpp!");

} // namespace naucrates::config

#endif // NAUCRATES_FIRMWARE_CONFIG_HPP
