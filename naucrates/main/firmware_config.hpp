#ifndef NAUCRATES_FIRMWARE_CONFIG_HPP
#define NAUCRATES_FIRMWARE_CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace naucrates::config {

struct W6300Config {
	uint8_t pin_miso = 19;
	uint8_t pin_cs   = 16;
	uint8_t pin_sck  = 17;
	uint8_t pin_mosi = 18;
	uint8_t pin_rst  = 22;
	uint8_t pin_int  = 15;
};

inline constexpr W6300Config W6300{};

template <uint8_t... Pins> consteval bool are_pins_unique() {
	constexpr std::array<uint8_t, sizeof...(Pins)> pin_array{Pins...};
	for (size_t i = 0; i < sizeof...(Pins); ++i) {
		for (size_t j = i + 1; j < sizeof...(Pins); ++j) {
			if (pin_array[i] == pin_array[j])
				return false;
		}
	}
	return true;
}

static_assert(are_pins_unique<W6300.pin_miso, W6300.pin_cs, W6300.pin_sck, W6300.pin_mosi,
                              W6300.pin_rst, W6300.pin_int>(),
              "FATAL BUILD ERROR: Duplicate GPIO pin assignment detected in firmware_config.hpp!");

struct TimerCfg {
	uint32_t timer     = 0;
	uint32_t alarm     = 0;
	int64_t  period_us = 0;
	uint32_t priority  = 0x80; // PICO_DEFAULT_IRQ_PRIORITY
};

namespace timer {

inline constexpr TimerCfg motion_executor{
    .timer = 0, .alarm = 0, .period_us = -5}; // 200kHz base thread (core 0)

inline constexpr TimerCfg general_io{
    .timer = 1, .alarm = 0, .period_us = -1000}; // 1 kHz servo thread (core 1)

inline constexpr TimerCfg test_core0{
    .timer = 0, .alarm = 1, .period_us = -1000}; // 1 kHz servo thread (core 1)

inline constexpr TimerCfg test_core1{
    .timer = 1, .alarm = 1, .period_us = -1000}; // 1 kHz servo thread (core 1)

} // namespace timer

} // namespace naucrates::config

#endif // NAUCRATES_FIRMWARE_CONFIG_HPP
