#include "naucrates/platform/interrupt_manager.hpp"

extern "C" void wiznet_gpio_isr()
{
    naucrates::InterruptManagerSingleton::instance()
        .dispatch<naucrates::IrqId::WiznetInt>();
}
