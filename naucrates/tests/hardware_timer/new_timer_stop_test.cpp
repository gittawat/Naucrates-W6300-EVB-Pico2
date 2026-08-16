#include <cstdint>

#include "naucrates/system/new_timer.hpp"
#include "pico/stdlib.h"

using namespace naucrates;

// 10 kHz burst: toggles GPIO0, returns false after 100 ticks -> must self-stop
static constexpr int64_t kBurstIntervalUs = -100;
static constexpr uint32_t kBurstTicks     = 100;

class BurstBlinker {
public:
	bool tick() noexcept {
		gpio_xor_mask(1u << 0);
		return (++count_ < kBurstTicks);
	}
	using timer_type = system::TimerIsrStatic<
	    config::TimerCfg{.timer = 0, .alarm = 0, .period_us = kBurstIntervalUs},
	    BurstBlinker, &BurstBlinker::tick>;

private:
	timer_type timer_;
	uint32_t   count_ = 0;

public:
	void init() noexcept { timer_.init(*this); }
	void start() noexcept {
		count_ = 0;
		timer_.start();
	}
	bool running() const noexcept { return timer_.running(); }
	void stop() noexcept { timer_type::stop(); }
};

int main() {

	gpio_init(0);
	gpio_set_dir(0, GPIO_OUT);
	gpio_put(0, false);

	BurstBlinker burst;
	burst.init();
	burst.start();

	// Burst #1: self-stop via `return false` after 100 ticks (10 ms)
	while (burst.running()) {
		tight_loop_contents();
	}
	busy_wait_ms(2);

	// Burst #2: external stop() after 5 ms
	burst.start();
	busy_wait_ms(5);
	burst.stop();

	while (true) {
		tight_loop_contents();
	}
}
