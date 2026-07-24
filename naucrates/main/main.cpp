#include "pico/stdlib.h"
#include "naucrates/platform/rtt_logger.hpp"
#include "naucrates/modules/module_concepts.hpp"
#include "naucrates/modules/shared_data.hpp"
#include "naucrates/modules/static_module_runner.hpp"
#include "naucrates/modules/udp_echo/udp_echo_module.hpp"
#include "naucrates/drivers/wiznet/w6300_driver.hpp"
#include "naucrates/main/firmware_config.hpp"

using namespace naucrates;

static drivers::wiznet::W6300Driver wiz;
static SharedData shared_data;
static UdpEchoModule echo(wiz, shared_data, config::UDP_ECHO);

static_assert(InterruptHandlingModule<UdpEchoModule>,
              "UdpEchoModule must implement void handle_interrupt()");

extern "C" void wiznet_gpio_isr()
{
    gpio_acknowledge_irq(PIN_INT, GPIO_IRQ_EDGE_FALL);
    echo.handle_interrupt();
}

int main()
{
    RTTLogger::init();
    RTTLogger::write("=== naucrates firmware ===\r\n");

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

    gpio_add_raw_irq_handler(PIN_INT, &wiznet_gpio_isr);
    gpio_set_irq_enabled(PIN_INT, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    echo.configure();

    static StaticModuleRunner runner(config::SERVO_FREQ_HZ, echo);

    RTTLogger::write("naucrates firmware initialized.\r\n\r\n");

    while (true)
    {
        runner.run_once();
    }
}
