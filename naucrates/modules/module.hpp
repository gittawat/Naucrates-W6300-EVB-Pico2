#ifndef NAUCRATES_MODULE_HPP
#define NAUCRATES_MODULE_HPP

#include <cstdint>
#include <cstddef>

namespace naucrates
{

class Module
{
public:
    Module() = default;
    explicit Module(int32_t thread_freq, int32_t slow_update_freq);
    virtual ~Module() = default;

    Module(const Module&)            = delete;
    Module& operator=(const Module&) = delete;
    Module(Module&&)                 = delete;
    Module& operator=(Module&&)      = delete;

    void run_module();
    void run_module_post();

    virtual void update() {}
    virtual void update_post() {}
    virtual void slow_update() {}
    virtual void configure() {}
    virtual void handle_interrupt() {}

private:
    int32_t thread_freq_       = 0;
    int32_t slow_update_freq_  = 0;
    int32_t update_count_      = 1;
    int32_t counter_           = 0;
};

} // namespace naucrates

#endif // NAUCRATES_MODULE_HPP
