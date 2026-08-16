#ifndef NAUCRATES_SYSTEM_TIMER_ISR_STATIC_HPP
#define NAUCRATES_SYSTEM_TIMER_ISR_STATIC_HPP

#include <cstdint>
#include <cstdlib>
#include <hardware/gpio.h>

#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"
#include "pico/platform.h"

namespace naucrates::system {

/// @brief Compile-time-dispatched hardware-alarm timer.
///
/// The owner class and the member callback are template parameters, so
/// the ISR calls `(Obj->*Callback)()` directly — no delegate, no
/// instance table, no `__get_current_exception()` lookup.  The member
/// function pointer is known at compile time and is inlined into the
/// ISR if its body is visible.
///
/// The timer instance is derived from the running core at runtime
/// (TIMER_INSTANCE(get_core_num()), matching
/// timer_isr_runtime_core_test.cpp); the constructor claims a free
/// alarm and computes the IRQ number exactly like the raw SDK test.
///
/// The full setup (claim the alarm, install the ISR, set the priority)
/// happens in the constructor.  The object must therefore be
/// constructed AFTER runtime_init — i.e. as a local in main() /
/// core1_entry(), not as a global — and on the core the ISR must fire
/// on, since each core has its own NVIC and vector table.
///
/// @note One object instance per (Class, Callback) pair: the timer
///       state is static and shared across all objects of the same
///       instantiation.
template <auto Obj, auto Callback> class TimerIsrStatic {
public:
	TimerIsrStatic(const TimerIsrStatic&)            = delete;
	TimerIsrStatic& operator=(const TimerIsrStatic&) = delete;

	/// Derives the timer instance from the running core, claims a free
	/// hardware alarm, registers the ISR and sets the priority.  On
	/// RP2350 core 0 owns timer0 and core 1 owns timer1 (matching
	/// timer_isr_runtime_core_test.cpp).  Panics on no free alarm.
	explicit TimerIsrStatic() {
		timer_       = TIMER_INSTANCE(get_core_num());
		alarm_index_ = static_cast<uint32_t>(timer_hardware_alarm_claim_unused(timer_, true));
		irq_         = timer_hardware_alarm_get_irq_num(timer_, alarm_index_);
	}

	~TimerIsrStatic() {
		irq_set_enabled(irq_, false);
		timer_->armed = 1u << alarm_index_;

		hw_clear_bits(&timer_->inte, 1u << alarm_index_);
		hw_clear_bits(&timer_->intr, 1u << alarm_index_);

		timer_hardware_alarm_unclaim(timer_, alarm_index_);
		irq_remove_handler(irq_, irq_handler);
	}

	/// Arms a repeating timer.  The callback returns false to stop the
	/// timer.  Returns true on success.
	bool start_repeating_us(int64_t period_us, uint32_t priority = PICO_DEFAULT_IRQ_PRIORITY) {
		if (period_us == 0) {
			return false;
		}

		magnitude = period_us < 0 ? static_cast<uint32_t>(-period_us) : static_cast<uint32_t>(period_us); 

		if (period_us > 0) {
			irq_handler = irq_handler_arm_after_cb;

		} else {
			irq_handler = irq_handler_arm_before_cb;
		}

		irq_set_exclusive_handler(irq_, irq_handler);
		irq_set_priority(irq_, priority);
		irq_set_enabled(irq_, true);
		running_ = true;

		// Unmask Alarm interrupt on TIMER
		hw_set_bits(&timer_->inte, 1u << alarm_index_);
		arm_next();
		return true;
	}
	bool running() const {
		return running_;
	}

	/// Disarms the alarm and marks the timer stopped.  Idempotent;
	/// safe to call from inside the callback.
	static void stop() {
		running_ = false;
		if (timer_ != nullptr) {
			timer_->armed = 1u << alarm_index_;
		}
	}

private:
	inline static timer_hw_t*   timer_       = nullptr;
	inline static uint32_t      alarm_index_ = 0;
	inline static uint32_t      irq_         = 0;
	inline static uint32_t      magnitude    = 0;
	inline static volatile bool running_     = false;
	inline static irq_handler_t irq_handler;


	static void __not_in_flash_func(irq_handler_arm_before_cb)() {
		//gpio_xor_mask(1u << 1); // probe: ISR entry (A/B parity with TimerIsrStatic)

		// Clear the interrupt flag
		hw_clear_bits(&timer_->intr, 1u << alarm_index_);

		timer_->armed = static_cast<uint32_t>(!running_) << alarm_index_;

		arm_next();
		// const bool repeat =
		(Obj->*Callback)();
	}

	static void __not_in_flash_func(irq_handler_arm_after_cb)() {
		//gpio_xor_mask(1u << 1); // probe: ISR entry (A/B parity with TimerIsrStatic)

		// Clear the interrupt flag
		hw_clear_bits(&timer_->intr, 1u << alarm_index_);


		timer_->armed = static_cast<uint32_t>(!running_) << alarm_index_;
		// const bool repeat =
		(Obj->*Callback)();

		arm_next();
	}

	__force_inline static void arm_next() {
		timer_->alarm[alarm_index_] = timer_->timerawl + magnitude;
	}
};

} // namespace naucrates::system

#endif // NAUCRATES_SYSTEM_TIMER_ISR_STATIC_HPP
