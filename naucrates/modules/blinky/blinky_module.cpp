#include "naucrates/modules/blinky/blinky_module.hpp"
#include "pico/stdlib.h"

namespace naucrates
{

static constexpr int32_t kServoFreqHz = config::SERVO_FREQ_HZ;

void BlinkyModule::configure()
{
    gpio_init(led_pin_);
    gpio_set_dir(led_pin_, GPIO_OUT);
    gpio_put(led_pin_, false);

    period_ticks_ = kServoFreqHz / toggle_freq_hz_ / 2;
    if (period_ticks_ == 0)
    {
        period_ticks_ = 1;
    }

    tick_count_ = 0;
    state_      = false;
}

void BlinkyModule::update()
{
    ++tick_count_;
    if (tick_count_ >= period_ticks_)
    {
        tick_count_ = 0;
        state_ = !state_;
        gpio_put(led_pin_, state_);
    }
}

} // namespace naucrates
