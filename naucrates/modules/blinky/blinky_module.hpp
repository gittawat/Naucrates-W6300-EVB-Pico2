#ifndef NAUCRATES_BLINKY_MODULE_HPP
#define NAUCRATES_BLINKY_MODULE_HPP

#include <cstdint>
#include "naucrates/main/firmware_config.hpp"

namespace naucrates
{

class BlinkyModule
{
public:
    explicit BlinkyModule(const config::BlinkyConfig& cfg)
        : led_pin_(cfg.led_pin)
        , toggle_freq_hz_(cfg.toggle_freq_hz)
        , tick_count_(0)
        , state_(false)
    {
    }

    void configure();
    void update();

private:
    uint8_t  led_pin_;
    uint32_t toggle_freq_hz_;
    uint32_t period_ticks_;
    uint32_t tick_count_;
    bool     state_;
};

} // namespace naucrates

#endif // NAUCRATES_BLINKY_MODULE_HPP
