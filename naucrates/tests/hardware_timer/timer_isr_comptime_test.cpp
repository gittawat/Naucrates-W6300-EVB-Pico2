#include <cstdint>

#include "naucrates/system/timer_isr_comptime.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
using namespace naucrates;

// 1 kHz on core 0 (timer 0, alarm 0)
static constexpr int64_t kIntervalUs = 100; // 1 ms

class Core0Blinker {
public:
	bool on_tick() {
		gpio_xor_mask(1u << 0); // core 0 ISR toggle GPIO pin 0 (callback marker)
		return true;
	}
};

class Core1Blinker {
public:
	bool on_tick() {
		gpio_xor_mask(1u << 1);
		return true;
	}
};

// MUST be global (static storage): NTTP requires a constant expression
// address for the object pointer.
static Core1Blinker blinker1;
static Core0Blinker blinker0;


// NOTE: the TimerIsrComptime objects are NOT global — their constructor
// performs the full setup (claim alarm, install ISR, set priority) which
// requires runtime_init to have run, so they must be constructed inside
// main() / core1_entry().

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);



	// Must run on core 1: NVIC is per-core.

	system::TimerIsrComptime<&blinker1, &Core1Blinker::on_tick, 1, 0> core1_timer(PICO_DEFAULT_IRQ_PRIORITY);
	//core1_timer.start_repeating_us(kIntervalUs/2);

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

	// Launch Core 1 execution
	multicore_launch_core1(core1_entry);
	multicore_fifo_pop_blocking();



	// Setup timer locally on Core 0 (timer 0, alarm 0) — constructor
	// does the full setup: claim alarm, install ISR, set priority.
	system::TimerIsrComptime<&blinker0, &Core0Blinker::on_tick, 0, 0> core0_timer(PICO_DEFAULT_IRQ_PRIORITY);
	core0_timer.start_repeating_us(kIntervalUs);

	while (true) {
		tight_loop_contents();
	}
}
