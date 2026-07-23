#ifndef NAUCRATES_MODULE_RUNNER_HPP
#define NAUCRATES_MODULE_RUNNER_HPP

#include <cstdint>
#include "etl/list.h"
#include "naucrates/modules/module.hpp"

namespace naucrates
{

class ModuleRunner
{
public:
    static constexpr size_t kMaxModules = 16;

    explicit ModuleRunner(int32_t frequency_hz) : frequency_hz_(frequency_hz) {}

    void register_module(Module& module);
    void register_module_post(Module& module);

    void run_once();

    int32_t frequency() const { return frequency_hz_; }

private:
    int32_t frequency_hz_;
    etl::list<Module*, kMaxModules> modules_{};
    etl::list<Module*, kMaxModules> post_modules_{};
};

} // namespace naucrates

#endif // NAUCRATES_MODULE_RUNNER_HPP
