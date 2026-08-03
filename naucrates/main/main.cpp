#include "pico/stdlib.h"
#include "pico/time.h"
#include "naucrates/utilities/rtt_logger.hpp"
#include "naucrates/features/blinky/blinky.hpp"

using namespace naucrates;

int main()
{
    RTTLogger::init();
    RTTLogger::write("=== naucrates firmware ===\r\n");

    features::Blinky blinky(25); // onboard LED
    blinky.init();

    RTTLogger::write("naucrates firmware initialized.\r\n\r\n");

    while (true)
    {
        blinky.toggle();
        sleep_ms(250);
    }
}
