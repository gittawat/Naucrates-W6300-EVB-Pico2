#ifndef NAUCRATES_STATIC_MODULE_RUNNER_HPP
#define NAUCRATES_STATIC_MODULE_RUNNER_HPP

#include <cstdint>
#include <tuple>
#include "pico/time.h"
#include "naucrates/modules/module_concepts.hpp"

namespace naucrates
{

template <ModuleConcept... Modules>
class StaticModuleRunner
{
public:
    explicit constexpr StaticModuleRunner(int32_t frequency_hz, Modules&... modules)
        : frequency_hz_(frequency_hz)
        , interval_us_(frequency_hz > 0 ? (1'000'000 / static_cast<uint32_t>(frequency_hz)) : 1000)
        , next_deadline_(time_us_32() + interval_us_)
        , modules_(modules...) {}

    void run_once()
    {
        while (static_cast<int32_t>(time_us_32() - next_deadline_) < 0)
        {
            tight_loop_contents();
        }

        std::apply([](auto&... mod) {
            (mod.update(), ...);
        }, modules_);

        next_deadline_ += interval_us_;
    }

    constexpr int32_t frequency() const { return frequency_hz_; }

private:
    int32_t frequency_hz_;
    uint32_t interval_us_;
    uint32_t next_deadline_;
    std::tuple<Modules&...> modules_;
};

} // namespace naucrates

#endif // NAUCRATES_STATIC_MODULE_RUNNER_HPP
