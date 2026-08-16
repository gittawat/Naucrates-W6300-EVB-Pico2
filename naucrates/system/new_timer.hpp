#ifndef NAUCRATES_SYSTEM_NEW_TIMER_HPP
#define NAUCRATES_SYSTEM_NEW_TIMER_HPP

#include <cstdint>
#include <cstdlib>
#include <etl/atomic.h>
#include <hardware/gpio.h>

#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"
#include "pico/platform.h"

#include "naucrates/main/firmware_config.hpp"

namespace naucrates::system {

// over engineered ? lmao YES, still not sure if stop work correcly or not but the test show
// expected result
template <config::TimerCfg timer_cfg, class Obj, bool (Obj::*TickFn)() noexcept>
class TimerIsrStatic {
	static_assert(timer_cfg.timer < NUM_GENERIC_TIMERS, "Timer index out of range");
	static_assert(timer_cfg.alarm < NUM_ALARMS, "Alarm index out of range");
	static_assert(timer_cfg.period_us != 0, "Timer period cannot be 0");

public:
	TimerIsrStatic()                                 = default;
	TimerIsrStatic(const TimerIsrStatic&)            = delete;
	TimerIsrStatic& operator=(const TimerIsrStatic&) = delete;
	TimerIsrStatic(TimerIsrStatic&&)                 = delete;
	TimerIsrStatic& operator=(TimerIsrStatic&&)      = delete;

	~TimerIsrStatic() {
		if (!initialized_) {
			return;
		}

		stop();

		irq_set_enabled(irq_, false);
		irq_remove_handler(irq_, irq_handler);

		timer_hardware_alarm_unclaim(timer(), alarm_index_);

		obj          = nullptr;
		initialized_ = false;
	}

	void init(Obj& o) noexcept {
		hard_assert(!initialized_ && "TimerIsr: init() called twice");
		hard_assert(get_core_num() == timer_cfg.timer &&
		            "TimerIsr: wrong core (timer0↔core0, timer1↔core1)");

		timer_hardware_alarm_claim(timer(), alarm_index_); // SDK panics if already claimed
		obj          = &o;
		initialized_ = true;
	}

	/// Arms a repeating timer.  The callback returns false to stop the
	/// timer.  Returns true on success.
	bool start() noexcept {
		if (!initialized_) {
			return false;
		}

		// Clear any pending interrupt flags before unmasking
		timer()->intr = 1u << alarm_index_;
		irq_clear(irq_);

		irq_set_exclusive_handler(irq_, irq_handler);
		irq_set_priority(irq_, timer_cfg.priority);
		irq_set_enabled(irq_, true);

		running_.store(true, etl::memory_order_release);
		hw_set_bits(&(timer()->inte), 1u << alarm_index_);
		arm_next();
		return true;
	}
	bool running() const noexcept {
		return running_.load(etl::memory_order_relaxed);
	}

	static void stop() noexcept {
		if (!initialized_) {
			return;
		}

		running_.store(false, etl::memory_order_release);
		hw_clear_bits(&(timer()->inte), 1u << alarm_index_);
		timer()->armed = 1u << alarm_index_;
		timer()->intr  = 1u << alarm_index_;
		irq_clear(irq_);
	}

private:
	inline static constinit Obj*        obj          = nullptr;
	inline static constexpr uint32_t    alarm_index_ = timer_cfg.alarm;
	inline static constexpr uint32_t    irq_ =
	    timer_cfg.timer == 0 ? TIMER0_IRQ_0 + alarm_index_ : TIMER1_IRQ_0 + alarm_index_;
	inline static constexpr uint32_t magnitude = timer_cfg.period_us < 0
	                                                 ? static_cast<uint32_t>(-timer_cfg.period_us)
	                                                 : static_cast<uint32_t>(timer_cfg.period_us);
	inline static etl::atomic<bool>  running_  = false;
	inline static bool               initialized_ = false;

	static void __time_critical_func(irq_handler)() noexcept {
		gpio_xor_mask((1u << 1));
		// Clear interrupt flag
		timer()->intr = 1u << alarm_index_;

		if (!running_.load(etl::memory_order_relaxed) || obj == nullptr) {
			timer()->armed = 1u << alarm_index_;
			return;
		}

		if constexpr (timer_cfg.period_us < 0) {
			arm_next();
		}

		if (!(obj->*TickFn)()) {
			stop();
			return;
		}

		if constexpr (timer_cfg.period_us > 0) {
			if (running_.load(etl::memory_order_relaxed)) {
				arm_next();
			}
		}
	}

	__force_inline static void arm_next() {
		timer()->alarm[alarm_index_] = timer()->timerawl + magnitude;
	}

	[[nodiscard]] __force_inline static timer_hw_t* timer() noexcept {
		return (timer_cfg.timer == 0) ? timer0_hw : timer1_hw;
	}
};

} // namespace naucrates::system

#endif // NAUCRATES_SYSTEM_NEW_TIMER_HPP
