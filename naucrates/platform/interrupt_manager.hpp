#ifndef NAUCRATES_INTERRUPT_MANAGER_HPP
#define NAUCRATES_INTERRUPT_MANAGER_HPP

#include <cstddef>

#include "etl/delegate.h"
#include "etl/delegate_service.h"
#include "etl/singleton.h"

namespace naucrates
{

enum class IrqId : size_t
{
    WiznetInt = 0,
    COUNT
};

class InterruptManager
{
public:
    using Handler = etl::delegate<void(size_t)>;

    InterruptManager() = default;

    InterruptManager(const InterruptManager&)            = delete;
    InterruptManager& operator=(const InterruptManager&) = delete;
    InterruptManager(InterruptManager&&)                 = delete;
    InterruptManager& operator=(InterruptManager&&)      = delete;

    template <IrqId Id>
    void register_handler(Handler handler)
    {
        dispatcher_.register_delegate<static_cast<size_t>(Id)>(handler);
    }

    void register_handler(IrqId id, Handler handler)
    {
        dispatcher_.register_delegate(static_cast<size_t>(id), handler);
    }

    void register_unhandled(Handler handler)
    {
        dispatcher_.register_unhandled_delegate(handler);
    }

    template <IrqId Id>
    void dispatch() const
    {
        dispatcher_.call<static_cast<size_t>(Id)>();
    }

    void dispatch(IrqId id) const
    {
        dispatcher_.call(static_cast<size_t>(id));
    }

private:
    static constexpr size_t kRange  = static_cast<size_t>(IrqId::COUNT);
    static constexpr size_t kOffset = 0;

    etl::delegate_service<kRange, kOffset> dispatcher_{};
};

using InterruptManagerSingleton = etl::singleton<InterruptManager>;

} // namespace naucrates

#endif // NAUCRATES_INTERRUPT_MANAGER_HPP
