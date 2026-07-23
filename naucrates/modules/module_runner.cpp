#include "naucrates/modules/module_runner.hpp"

namespace naucrates
{

void ModuleRunner::register_module(Module& module)
{
    modules_.push_back(&module);
}

void ModuleRunner::register_module_post(Module& module)
{
    post_modules_.push_back(&module);
}

void ModuleRunner::run_once()
{
    for (auto it = modules_.begin(); it != modules_.end(); ++it)
    {
        (*it)->run_module();
    }
    for (auto it = post_modules_.begin(); it != post_modules_.end(); ++it)
    {
        (*it)->run_module_post();
    }
}

} // namespace naucrates
