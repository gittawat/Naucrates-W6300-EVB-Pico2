#ifndef NAUCRATES_BLINKY_HPP
#define NAUCRATES_BLINKY_HPP

#include <cstdint>

namespace naucrates::features
{

class Blinky
{
public:
    explicit Blinky(uint8_t led_pin);

    void init();   // configure pin as GPIO output, start off
    void toggle(); // flip the LED state — caller owns the timing

private:
    uint8_t led_pin_;
};

} // namespace naucrates::features

#endif // NAUCRATES_BLINKY_HPP
