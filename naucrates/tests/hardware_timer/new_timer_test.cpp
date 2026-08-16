#include <cstdint>

#include "naucrates/system/new_timer.hpp"
#include "pico/multicore.h"
#include "pico/stdlib.h"

using namespace naucrates;

static constexpr int64_t kIntervalUs      = -100; // 10 kHz, start-to-start
static constexpr int64_t kFastIntervalUs  = -10;   // 100 kHz, start-to-start

// --- core 0: continuous 1 kHz on timer0/alarm0 -> GPIO0 ---
class Core0Blinker {
public:
	bool tick() noexcept { // must precede timer_type (PMF ordering)
		gpio_xor_mask(1u << 0);
		return true;
	}
	using timer_type = system::TimerIsrStatic<
	    config::TimerCfg{.timer = 0, .alarm = 0, .period_us = kIntervalUs},
	    Core0Blinker, &Core0Blinker::tick>;

private:
	timer_type timer_;

public:
	void init() noexcept { timer_.init(*this); }
	void start() noexcept { timer_.start(); }
};


// --- core 1: continuous 100 kHz on timer1/alarm0 -> GPIO1 ---
class Core1Blinker {
public:
	bool tick() noexcept {
		gpio_xor_mask(1u << 1);
		return true;
	}
	using timer_type = system::TimerIsrStatic<
	    config::TimerCfg{.timer = 1, .alarm = 0, .period_us = kFastIntervalUs},
	    Core1Blinker, &Core1Blinker::tick>;

private:
	timer_type timer_;

public:
	void init() noexcept { timer_.init(*this); }
	void start() noexcept { timer_.start(); }
};

// Waits until the timer stops (running()==false) with a deadline.
// GPIO3 = pass/fail: high = stopped in time (PASS), low = FAIL.

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);

	Core1Blinker blinker; // must run on core 1 (init() asserts it)
	blinker.init();
	//blinker.start();

	while (true) {
		tight_loop_contents();
	}
}

int main() {
	for (uint gp = 0; gp < 4; ++gp) {
		gpio_init(gp);
		gpio_set_dir(gp, GPIO_OUT);
		gpio_put(gp, false);
	}

	Core0Blinker blinker;
	blinker.init(); // core 0
	blinker.start();

	multicore_launch_core1(core1_entry);
	static_cast<void>(multicore_fifo_pop_blocking());

	while (true) {
		tight_loop_contents();
	}
}
