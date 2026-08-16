#ifndef NAUCRATES_SYSTEM_TIMER_ISR_COMPTIME_HPP
#define NAUCRATES_SYSTEM_TIMER_ISR_COMPTIME_HPP

#include <cstdint>

#include <hardware/gpio.h>

#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"
#include "pico/platform.h"

namespace naucrates::system {

namespace {

/// @brief Fully compile-time-dispatched hardware-alarm timer.
///
/// Everything that can be known at build time is baked in: the owner
/// object (NTTP, must have static storage duration), the member
/// callback, the timer number and the alarm number.  The ISR therefore
/// contains no runtime loads for timer_/alarm_index_/obj_ — the
/// peripheral address and mask are compile-time constants, and the
/// callback call is a direct inlined call on a comptime object.
///
/// The full setup (claim the fixed alarm, install the handler, set the
/// priority) happens in the constructor.  The object must therefore be
/// constructed AFTER runtime_init — i.e. as a local in main() /
/// core1_entry(), not as a global.
///
/// This is the A/B comparison variant of TimerIsrStatic for measuring
/// dispatch latency: the probe at ISR entry is placed identically to
/// the baseline, so the GPIO1(entry)->GPIO0(callback) gap can be
/// compared directly.
template <auto Obj, auto Callback, uint32_t TimerNum, uint32_t AlarmNum> class TimerIsrComptime {
	static_assert(TimerNum < NUM_GENERIC_TIMERS,
	              "TimerIsrComptime: timer index out of range (0..1)");
	static_assert(AlarmNum < NUM_ALARMS, "TimerIsrComptime: alarm index out of range (0..3)");
	static_assert(TIMER0_IRQ_3 == TIMER0_IRQ_0 + 3, "TimerIsrComptime: TIMER0 IRQs not contiguous");
	static_assert(TIMER1_IRQ_3 == TIMER1_IRQ_0 + 3, "TimerIsrComptime: TIMER1 IRQs not contiguous");

public:
	TimerIsrComptime()                                   = default;
	TimerIsrComptime(const TimerIsrComptime&)            = delete;
	TimerIsrComptime& operator=(const TimerIsrComptime&) = delete;

	/// Claims the fixed alarm, registers the ISR and sets the
	/// priority.  Must be constructed on the core matching TimerNum
	/// (timer0 -> core0, timer1 -> core1).  Panics on wrong core or
	/// conflicting claim.
	explicit TimerIsrComptime(uint32_t priority) {
		hard_assert(get_core_num() == TimerNum && "TimerIsrComptime: constructed on wrong core");

		timer_hardware_alarm_claim(TIMER_INSTANCE(TimerNum), kAlarm);
		irq_set_exclusive_handler(kIrq, irq_handler);
		irq_set_priority(kIrq, priority);
	}

	~TimerIsrComptime() {
		irq_set_enabled(kIrq, false);
		TIMER_INSTANCE(TimerNum)->armed = 1u << kAlarm;

		hw_clear_bits(&TIMER_INSTANCE(TimerNum)->inte, 1u << kAlarm);
		hw_clear_bits(&TIMER_INSTANCE(TimerNum)->intr, 1u << kAlarm);

		timer_hardware_alarm_unclaim(TIMER_INSTANCE(TimerNum), kAlarm);
		irq_remove_handler(kIrq, irq_handler);
	}

	/// Arms a repeating timer.  The callback returns false to stop the
	/// timer.  Returns true on success.
	bool start_repeating_us(int64_t period_us) {
		if (period_us == 0) {
			return false;
		}
		period_us_ = period_us;
		running_   = true;

		irq_set_enabled(kIrq, true);
		// Unmask Alarm interrupt on TIMER
		hw_set_bits(&TIMER_INSTANCE(TimerNum)->inte, 1u << kAlarm);
		arm_next();
		return true;
	}
	bool running() const {
		return running_;
	}

	/// Disarms the alarm and marks the timer stopped.  Idempotent;
	/// safe to call from inside the callback.
	static void stop() {
		running_                        = false;
		TIMER_INSTANCE(TimerNum)->armed = 1u << kAlarm;
	}

private:
	static constexpr uint32_t kAlarm = AlarmNum;
	static constexpr uint32_t kIrq =
	    TimerNum == 0 ? TIMER0_IRQ_0 + AlarmNum : TIMER1_IRQ_0 + AlarmNum;

	inline static int64_t       period_us_ = 0;
	inline static volatile bool running_   = false;

	// __not_in_flash_func() derives the section name from the function
	// name, so every instantiation places its ISR in the same
	// ".time_critical.irq_handler" section.  GCC emits some
	// instantiations weak/COMDAT (e.g. when the NTTP object lives in a
	// non-static function) and others strong, and the mixed section
	// flags make the shared section name a hard error ("section type
	// conflict").  The class therefore lives in an anonymous namespace
	// so every instantiation has TU-local linkage and is emitted
	// strong, giving all ISRs matching flags for the shared section.
	static void __not_in_flash_func(irq_handler)() {
		gpio_xor_mask(1u << 1); // probe: isr entry (a/b parity with timerisrstatic)

		// Clear the interrupt flag
		hw_clear_bits(&TIMER_INSTANCE(TimerNum)->intr, 1u << kAlarm);

		//if (!running_) {
		//	TIMER_INSTANCE(TimerNum)->armed = 1u << kAlarm;
		//	return;
		//}

		// Periods use a self-timed chain: the alarm is re-armed with
		// timerawl + |period|.  Negative periods re-arm at ISR entry
		// (start-to-start); positive periods re-arm after the callback
		// returns (matching the SDK repeating-timer semantics).
		//const bool rearm_at_entry = period_us_ < 0;
		//if (rearm_at_entry) {
			arm_next();
		//}

		//const bool repeat = 
		(Obj->*Callback)();
		//if (!repeat) {
		//	stop();
		//	return;
		//}

		//if (!rearm_at_entry) {
		//	arm_next();
		//}
	}

	__force_inline static void arm_next() {
		//const uint32_t magnitude =
		//    period_us_ < 0 ? static_cast<uint32_t>(-period_us_) : static_cast<uint32_t>(period_us_);
		TIMER_INSTANCE(TimerNum)->alarm[kAlarm] = TIMER_INSTANCE(TimerNum)->timerawl + static_cast<uint32_t>(period_us_) ;
	}
};

} // namespace

} // namespace naucrates::system

#endif // NAUCRATES_SYSTEM_TIMER_ISR_COMPTIME_HPP
