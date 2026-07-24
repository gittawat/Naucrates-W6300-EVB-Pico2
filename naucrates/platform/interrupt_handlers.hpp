#ifndef NAUCRATES_INTERRUPT_HANDLERS_HPP
#define NAUCRATES_INTERRUPT_HANDLERS_HPP

#include "naucrates/modules/module_concepts.hpp"

namespace naucrates::irq
{

template <typename T>
constexpr void check_interrupt_handler()
{
    static_assert(InterruptHandlingModule<T>,
                  "Instance must implement void handle_interrupt()");
}

} // namespace naucrates::irq

#endif // NAUCRATES_INTERRUPT_HANDLERS_HPP
