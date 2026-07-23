#include "naucrates/platform/interrupt_handlers.hpp"

namespace naucrates::irq
{

static IrqCallback g_wiznet_cb{};

void set_wiznet_handler(IrqCallback callback)
{
    g_wiznet_cb = callback;
}

extern "C" void wiznet_gpio_isr()
{
    if (g_wiznet_cb.is_valid())
    {
        g_wiznet_cb();
    }
}

} // namespace naucrates::irq
