#include <cstdint>

#include "naucrates/system/timer.hpp"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/stdlib.h"
using namespace naucrates;

// --- Resource claims: ONE per owner class, at file scope ---
// A second claim of the same (timer, alarm) in this TU is a compiler
// "redefinition" error; in another TU it is a duplicate-symbol link
// error.  Using a timer without any claim is an undefined-reference
// link error.  (timer0 -> core0, timer1 -> core1 is enforced by a
// static_assert inside Timer.)
NCR_CLAIM_TIMER(0, 0) // stepgen          (core 0, timer 0, alarm 0)
NCR_CLAIM_TIMER(0, 1) // general_io_check (core 0, timer 0, alarm 1)
NCR_CLAIM_TIMER(1, 0) // core1_thing      (core 1, timer 1, alarm 0)

class stepgen {
public:
	bool start() {
		if (!timer_.init()) {
			return false;
		}
		return timer_.start_repeating_us(-kPeriodUs, etl::delegate<bool()>(+[]() -> bool {
			gpio_xor_mask(1u << 0); // 1 kHz toggle on GPIO 0
			return true;
		}));
	}

private:
	static constexpr int64_t kPeriodUs = 1000;
	system::Timer<0, 1, 0>   timer_;
};

class general_io_check {
public:
	bool start() {
		if (!timer_.init()) {
			return false;
		}
		return timer_.start_repeating_us(-kPeriodUs, etl::delegate<bool()>(+[]() -> bool {
			gpio_xor_mask(1u << 2); // 2 kHz toggle on GPIO 2
			return true;
		}));
	}

private:
	static constexpr int64_t kPeriodUs = 500;
	system::Timer<0, 1, 0>   timer_;
};

class core1_thing {
public:
	bool start() {
		if (!timer_.init()) {
			return false;
		}
		return timer_.start_repeating_us(-kPeriodUs, etl::delegate<bool()>(+[]() -> bool {
			gpio_xor_mask(1u << 1); // 10 kHz toggle on GPIO 1
			return true;
		}));
	}

private:
	static constexpr int64_t kPeriodUs = 10;
	system::Timer<1, 0, 1>   timer_;
};

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);

	// Must run on core 1: NVIC is per-core, Timer<1,0,1> asserts it.
	core1_thing thing;
	thing.start();

	while (true) {
		tight_loop_contents();
	}
}

int main() {
	stdio_init_all();

	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_init(1);
	gpio_set_dir(1, GPIO_OUT);
	gpio_init(2);
	gpio_set_dir(2, GPIO_OUT);

	multicore_launch_core1(core1_entry);
	static_cast<void>(multicore_fifo_pop_blocking());

	// Must run on core 0.
	stepgen          gen;
	general_io_check gio;
	gen.start();
	gio.start();

	while (true) {
		tight_loop_contents();
	}
}

#if 0
// --- Both of the following must NOT compile ---

// 1. Claiming (0,0) twice in this TU:
//    error: redefinition of 'Timer<0u, 0u, 0u>::Timer()'
NCR_CLAIM_TIMER(0, 0)

// 2. Pairing timer 0 with core 1:
//    error: static assertion failed: Timer: timer N must be used on core N
class bad_core {
	system::Timer<0, 0, 1> timer_;
};
#endif
