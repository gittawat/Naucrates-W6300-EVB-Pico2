#ifndef NAUCRATES_INTERRUPT_HANDLERS_HPP
#define NAUCRATES_INTERRUPT_HANDLERS_HPP

#include "etl/delegate.h"

namespace naucrates::irq
{

using IrqCallback = etl::delegate<void()>;

void set_wiznet_handler(IrqCallback callback);

extern "C" void wiznet_gpio_isr();

} // namespace naucrates::irq

#endif // NAUCRATES_INTERRUPT_HANDLERS_HPP
