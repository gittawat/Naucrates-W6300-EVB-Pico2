#ifndef NAUCRATES_SYSTEM_TIMER_HPP
#define NAUCRATES_SYSTEM_TIMER_HPP

#include <cstdint>

#include "etl/delegate.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"
#include "pico/platform.h"

namespace naucrates::system {

/// @brief Compile-time-typed hardware-alarm timer.
///
/// The (timer, alarm, core) triple is part of the type, so a resource
/// conflict is a compile/link error instead of a runtime surprise.
///
/// The default constructor is declared but never defined generically:
/// the ONLY definitions are written by NCR_CLAIM_TIMER() in
/// naucrates::system.  Therefore:
///   - using Timer<T,A,C> without a claim  -> undefined reference at link
///   - claiming (T,A) twice in one TU      -> redefinition at compile
///   - claiming (T,A) in two TUs           -> duplicate symbol at link
///
/// Claim once per owner class, at file scope:
///   NCR_CLAIM_TIMER(0, 0)
///   class stepgen { system::Timer<0, 0, 0> timer_; };
template <uint32_t TimerNum, uint32_t AlarmNum, uint32_t CoreNum>
class Timer {
	static_assert(TimerNum < NUM_GENERIC_TIMERS, "Timer: timer index out of range (0..1)");
	static_assert(AlarmNum < NUM_ALARMS, "Timer: alarm index out of range (0..3)");
	static_assert(CoreNum == TimerNum, "Timer: timer N must be used on core N (timer0->core0, timer1->core1)");

public:
	Timer();
	Timer(const Timer&)            = delete;
	Timer& operator=(const Timer&) = delete;

	~Timer() {
		if (!initialized_) {
			return;
		}
		irq_set_enabled(irq_, false);
		timer_->armed = 1u << alarm_index;

		s_instance = nullptr;

		hw_clear_bits(&timer_->inte, 1u << alarm_index);
		hw_clear_bits(&timer_->intr, 1u << alarm_index);

		timer_hardware_alarm_unclaim(timer_, alarm_index);
		irq_remove_handler(irq_, irq_handler);

		initialized_ = false;
	}

	/// Claims the hardware alarm, registers the ISR and enables the IRQ.
	/// Must run on CoreNum (NVIC is per-core).  Panics on wrong core,
	/// conflicting claim or no free alarm.  Returns true on success.
	bool init(uint32_t priority = PICO_DEFAULT_IRQ_PRIORITY) {
		hard_assert(!initialized_ && "Timer: init() called twice");
		hard_assert(get_core_num() == CoreNum && "Timer: init() on wrong core");

		timer_      = TIMER_INSTANCE(TimerNum);
		alarm_index = static_cast<uint32_t>(timer_hardware_alarm_claim_unused(timer_, true));
		irq_        = timer_hardware_alarm_get_irq_num(timer_, alarm_index);

		hard_assert(irq_ < kMaxInstances && "Timer: alarm IRQ out of range");
		hard_assert(s_instance == nullptr && "Timer: alarm already registered");

		s_instance = this;
		irq_set_exclusive_handler(irq_, irq_handler);
		irq_set_priority(irq_, priority);
		initialized_ = true;
		return initialized_;
	}

	/// Arms a repeating timer.  cb must be valid and period_us != 0.
	/// The callback returns false to stop the timer.  Returns true on
	/// success.
	bool start_repeating_us(int64_t period_us, etl::delegate<bool()> cb) {
		if (!initialized_) {
			return false;
		}
		if (!cb.is_valid() || period_us == 0) {
			return false;
		}
		period_us_ = period_us;
		cb_        = cb;
		running_   = true;

		irq_set_enabled(irq_, true);
		hw_set_bits(&timer_->inte, 1u << alarm_index);
		arm_next();
		return true;
	}
	bool running() const {
		return running_;
	}

	/// Disarms the alarm and marks the timer stopped.  Idempotent;
	/// safe to call from inside the callback.
	void stop() {
		running_ = false;
		if (timer_ != nullptr) {
			timer_->armed = 1u << alarm_index;
		}
	}

private:
	static constexpr uint32_t kMaxInstances = NUM_GENERIC_TIMERS * NUM_ALARMS;
	inline static Timer*      s_instance    = nullptr;

	timer_hw_t*           timer_        = nullptr;
	etl::delegate<bool()> cb_;
	int64_t               period_us_   = 0;
	uint32_t              irq_         = 0;
	uint32_t              alarm_index  = 0;
	volatile bool         running_     = false;
	bool                  initialized_ = false;

	static void irq_handler() {
		Timer* self = s_instance;
		if (self == nullptr) {
			return;
		}

		hw_clear_bits(&self->timer_->intr, 1u << self->alarm_index);

		if (!(self->running_) || !(self->cb_.is_valid())) {
			self->stop();
			return;
		}

		const bool rearm_at_entry = self->period_us_ < 0;
		if (rearm_at_entry) {
			self->arm_next();
		}
		const bool repeat = self->cb_();
		if (!repeat) {
			self->stop();
			return;
		}
		if (!rearm_at_entry) {
			self->arm_next();
		}
	}
	void arm_next() const {
		const uint32_t magnitude =
		    period_us_ < 0 ? static_cast<uint32_t>(-period_us_) : static_cast<uint32_t>(period_us_);
		timer_->alarm[alarm_index] = timer_->timerawl + magnitude;
	}
};

// Pre-declare the explicit specializations of the default constructor so
// uses can appear anywhere in the TU without implicit instantiation:
template <> Timer<0, 0, 0>::Timer();
template <> Timer<0, 1, 0>::Timer();
template <> Timer<0, 2, 0>::Timer();
template <> Timer<0, 3, 0>::Timer();
template <> Timer<1, 0, 1>::Timer();
template <> Timer<1, 1, 1>::Timer();
template <> Timer<1, 2, 1>::Timer();
template <> Timer<1, 3, 1>::Timer();

} // namespace naucrates::system

/// @brief Claims hardware timer @p T, alarm @p A for one owner class.
///
/// Place ONE call at file scope in the owner's header/translation unit.
/// Expands to the sole definition of Timer<T,A,T>::Timer() — a strong
/// symbol, so a second claim (same TU or any TU) is an error.
#define NCR_CLAIM_TIMER(T, A) \
	namespace naucrates {     \
	namespace system {        \
	template <> Timer<T, A, T>::Timer() {} \
	}                         \
	}

#endif // NAUCRATES_SYSTEM_TIMER_HPP
