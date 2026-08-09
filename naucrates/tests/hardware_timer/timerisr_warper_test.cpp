#include <cstdint>

#include "hardware/gpio.h"
#include "naucrates/system/hardware_timer.hpp"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/platform.h"
using namespace naucrates;

// 1 kHz on core 0 (timer 0), 10 kHz on core 1 (timer 1)
static constexpr int64_t kIntervalUs     = 1000; // 1 ms
static constexpr int64_t kFastIntervalUs = 10;   // 10 us

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);

	// Must run on core 1: NVIC is per-core
	system::TimerIsr core1_timer;
	core1_timer.init({.timer_num = 1, .expected_core = 1});
	core1_timer.start_repeating_us(-kFastIntervalUs, etl::delegate<bool()>(+[]() -> bool {
		gpio_xor_mask(1u << 1); // core 1 ISR toggle GPIO pin 1
		return true;
	}));

	while (true) {
		tight_loop_contents();
	}
}

int main() {
	stdio_init_all();

	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_put(0, false);
	gpio_init(1);
	gpio_set_dir(1, GPIO_OUT);
	gpio_put(1, false);

	multicore_launch_core1(core1_entry);
	static_cast<void>(multicore_fifo_pop_blocking());

	// Must run on core 0
	system::TimerIsr core0_timer;
	core0_timer.init({.timer_num = 0, .expected_core = 0});
	core0_timer.start_repeating_us(-kIntervalUs, etl::delegate<bool()>(+[]() -> bool {
		gpio_xor_mask(1u << 0); // core 0 ISR toggle GPIO pin 0
		//gpio_put(0,false);	
		return true;
	}));

	while (true) {
		tight_loop_contents();
	}
}
