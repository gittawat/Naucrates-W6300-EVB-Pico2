#include "pico/stdlib.h"
#include "naucrates/platform/rtt_logger.hpp"
#include "naucrates/modules/static_module_runner.hpp"
#include "naucrates/modules/blinky/blinky_module.hpp"
#include "naucrates/main/firmware_config.hpp"

using namespace naucrates;

int main()
{
    RTTLogger::init();
    RTTLogger::write("=== naucrates firmware ===\r\n");

    BlinkyModule blinky(config::BLINKY);

    StaticModuleRunner runner(config::SERVO_FREQ_HZ, std::move(blinky));

    runner.configure();

    RTTLogger::write("naucrates firmware initialized.\r\n\r\n");

    while (true)
    {
        runner.run_once();
    }
}
