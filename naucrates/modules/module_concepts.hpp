#ifndef NAUCRATES_MODULE_CONCEPTS_HPP
#define NAUCRATES_MODULE_CONCEPTS_HPP

#include <concepts>

namespace naucrates
{

template <typename T>
concept ModuleConcept = requires(T& m) {
    m.update();
};

template <typename T>
concept ConfigurableModule = ModuleConcept<T> && requires(T& m) {
    m.configure();
};

template <typename T>
concept InterruptHandlingModule = requires(T& m) {
    m.handle_interrupt();
};

} // namespace naucrates

#endif // NAUCRATES_MODULE_CONCEPTS_HPP
