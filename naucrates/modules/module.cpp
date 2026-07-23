#include "naucrates/modules/module.hpp"

namespace naucrates
{

Module::Module(int32_t thread_freq, int32_t slow_update_freq)
    : thread_freq_(thread_freq)
    , slow_update_freq_(slow_update_freq)
{
    update_count_ = (slow_update_freq > 0) ? (thread_freq / slow_update_freq) : 1;
    if (update_count_ < 1) update_count_ = 1;
}

void Module::run_module()
{
    ++counter_;
    if (counter_ >= update_count_)
    {
        slow_update();
        counter_ = 0;
    }
    update();
}

void Module::run_module_post()
{
    update_post();
}

} // namespace naucrates
