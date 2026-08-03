#include "naucrates/features/blinky/blinky.hpp"

#include "hardware/gpio.h"

namespace naucrates::features
{

Blinky::Blinky(uint8_t led_pin)
    : led_pin_(led_pin)
{
}

void Blinky::init()
{
    gpio_init(led_pin_);
    gpio_set_dir(led_pin_, GPIO_OUT);
    gpio_put(led_pin_, false);
}

void Blinky::toggle()
{
    gpio_xor_mask(1u << led_pin_);
}

} // namespace naucrates::features
