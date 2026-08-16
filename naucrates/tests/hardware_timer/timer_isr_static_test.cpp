#include <cstdint>
#include <hardware/irq.h>

#include "naucrates/system/timer_isr_static.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
using namespace naucrates;

// 1 kHz on core 0 (timer 0), 10 kHz on core 1 (timer 1)
static constexpr int64_t kIntervalUs     = -1; // 1 us
static constexpr int64_t kFastIntervalUs = -2;   // 100 us

class Core0Blinker {
public:
	bool on_tick() {
		gpio_xor_mask(1u << 0); // core 0 ISR toggle GPIO pin 0
		return true;
	}
};

class Core1Blinker {
public:
	bool on_tick() {
		gpio_xor_mask(1u << 1); // core 1 ISR toggle GPIO pin 1
		return true;
	}
};



Core1Blinker blinker1;
Core0Blinker blinker0;



static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);


	// Must run on core 1: NVIC is per-core, timer derived from core.
	system::TimerIsrStatic<&blinker1, &Core1Blinker::on_tick> core1_timer;

	core1_timer.start_repeating_us(kFastIntervalUs);

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

	system::TimerIsrStatic  <&blinker0, &Core0Blinker::on_tick> core0_timer;
	// Launch Core 1 execution
	multicore_launch_core1(core1_entry);
	static_cast<void>(multicore_fifo_pop_blocking());

	// Setup timer locally on Core 0 (timer derived from core)
	core0_timer.start_repeating_us(kIntervalUs,PICO_DEFAULT_IRQ_PRIORITY);

	while (true) {
		tight_loop_contents();
	}
}
