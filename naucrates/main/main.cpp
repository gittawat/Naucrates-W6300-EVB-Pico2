#include "pico/stdlib.h"
#include "naucrates/platform/interrupt_manager.hpp"
#include "naucrates/platform/rtt_logger.hpp"
#include "naucrates/modules/module_runner.hpp"
#include "naucrates/modules/shared_data.hpp"
#include "naucrates/modules/udp_echo/udp_echo_module.hpp"
#include "naucrates/drivers/wiznet/w6300_driver.hpp"
#include "naucrates/main/firmware_config.hpp"

int main()
{
    using namespace naucrates;

    InterruptManagerSingleton::create();
    RTTLogger::init();
    RTTLogger::write("=== naucrates firmware ===\r\n");

    static drivers::wiznet::W6300Driver wiz;
    if (!wiz.init())
    {
        RTTLogger::write("FATAL: W6300 init failed!\r\n");
        while (true) { tight_loop_contents(); }
    }
    wiz.configure_network(config::UDP_ECHO.net_info);
    if (!wiz.open_udp_socket(config::UDP_ECHO.socket_id, config::UDP_ECHO.port))
    {
        RTTLogger::write("FATAL: socket open failed!\r\n");
        while (true) { tight_loop_contents(); }
    }

    static SharedData shared_data;
    static ModuleRunner runner(config::SERVO_FREQ_HZ);

    static UdpEchoModule echo(wiz, shared_data, config::UDP_ECHO);
    echo.configure();
    runner.register_module(echo);

    RTTLogger::write("naucrates firmware initialized.\r\n\r\n");

    while (true)
    {
        runner.run_once();
    }
}
