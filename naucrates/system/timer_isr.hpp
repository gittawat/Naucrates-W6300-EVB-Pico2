#ifndef NAUCRATES_SYSTEM_HARDWARE_TIMER_HPP
#define NAUCRATES_SYSTEM_HARDWARE_TIMER_HPP

#include <cstdint>
#include <etl/array.h>

#include "etl/delegate.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "pico/assert.h"
#include "hardware/gpio.h"
namespace naucrates::system {

/// @brief Raw hardware-alarm periodic ISR for one (timer, alarm) pair.
///
/// Claims a hardware timer alarm through the SDK claim table and runs a
/// user callback from the alarm IRQ.  The IRQ is enabled on the core
/// that calls init(), so each core gets its own instance.
///
/// Periods use a self-timed chain: the alarm is re-armed with
/// timerawl + |period|, so consecutive fires are exactly |period| us
/// apart with zero cumulative drift as long as the callback finishes
/// within the period.  Negative periods re-arm at ISR entry
/// (start-to-start); positive periods re-arm after the callback
/// returns (matching the SDK repeating-timer semantics).
///
/// @note init() MUST run on the core the ISR must fire on — each core
///       has its own NVIC and vector table.
class TimerIsr {
public:
	struct Config {
		uint32_t timer_num     = 0; ///< 0 → timer0_hw, 1 → timer1_hw
		uint32_t expected_core = 0; ///< core that must call init()
		uint32_t priority      = PICO_DEFAULT_IRQ_PRIORITY;
	};

	TimerIsr()                           = default;
	TimerIsr(const TimerIsr&)            = delete;
	TimerIsr& operator=(const TimerIsr&) = delete;

	~TimerIsr();

	/// Claims a hardware alarm, registers the ISR and enables the IRQ.
	/// Panics on wrong core, invalid timer, conflicting claim or no
	/// free alarm.  Returns true on success.
	bool init(const Config& cfg);

	/// Arms a repeating timer.  cb must be valid and period_us != 0.
	/// The callback returns false to stop the timer.  Returns true on
	/// success.
	bool start_repeating_us(int64_t period_us, etl::delegate<bool()> cb);

	bool running() const {
		return running_;
	}

	/// Disarms the alarm and marks the timer stopped.  Idempotent;
	/// safe to call from inside the callback.
	void stop();

private:
	static constexpr uint32_t kMaxInstances = NUM_GENERIC_TIMERS * NUM_ALARMS;
	inline static etl::array<TimerIsr*, kMaxInstances> s_instances{};

	static void irq_handler();

	void arm_next() const;

	timer_hw_t*           timer_ = nullptr;
	etl::delegate<bool()> cb_;
	int64_t               period_us_   = 0;
	uint32_t              irq_         = 0;
	uint32_t              alarm_index  = 0;
	bool                  running_     = false;
	bool                  initialized_ = false;
};

// inline TimerIsr* TimerIsr::s_instances[kMaxInstances] = {};

inline bool TimerIsr::init(const Config& cfg) {
	hard_assert(!initialized_ && "TimerIsr: init() called twice");
	hard_assert(cfg.timer_num < NUM_GENERIC_TIMERS && "TimerIsr: timer_num out of range");
	hard_assert(get_core_num() == cfg.expected_core && "TimerIsr: init() on wrong core");

	timer_      = TIMER_INSTANCE(cfg.timer_num);
	alarm_index = static_cast<uint32_t>(timer_hardware_alarm_claim_unused(timer_, true));
	irq_        = timer_hardware_alarm_get_irq_num(timer_, alarm_index);

	hard_assert(irq_ < kMaxInstances && "TimerIsr: alarm IRQ out of range");
	hard_assert(s_instances[irq_] == nullptr && "TimerIsr: alarm already registered");

	s_instances[irq_] = this;
	irq_set_exclusive_handler(irq_, irq_handler);
	irq_set_priority(irq_, cfg.priority);
	irq_set_enabled(irq_, true);
	initialized_ = true;
	return initialized_;
}

inline void TimerIsr::arm_next() const {
	const uint32_t magnitude =
	    period_us_ < 0 ? static_cast<uint32_t>(-period_us_) : static_cast<uint32_t>(period_us_);
	timer_->alarm[alarm_index] = timer_->timerawl + magnitude;
}

inline bool TimerIsr::start_repeating_us(int64_t period_us, etl::delegate<bool()> cb) {
	if (!initialized_) {
		return false;
	}
	if (!cb.is_valid() || period_us == 0) {
		return false;
	}
	period_us_ = period_us;
	cb_        = cb;
	running_   = true;

	// Unmask Alarm_index interrupt on TIMER
	hw_set_bits(&timer_->inte, 1u << alarm_index);
	arm_next();
	return true;
}

inline void TimerIsr::stop() {
	running_ = false;
	if (timer_ != nullptr) {
		timer_->armed = 1u << alarm_index;
	}
}

inline TimerIsr::~TimerIsr() {
	if (!initialized_) {
		return;
	}
	irq_set_enabled(irq_, false);
	timer_->armed = 1u << alarm_index;

	hw_clear_bits(&timer_->inte, 1u << alarm_index);
	timer_hardware_alarm_unclaim(timer_, alarm_index);
	irq_remove_handler(irq_, irq_handler);

	s_instances[irq_] = nullptr;
	initialized_      = false;
}

inline void TimerIsr::irq_handler() {
	// for time measuring
	// gpio_put(0, true);
	// gpio_put(1, true);

	const uint32_t irq  = __get_current_exception() - VTABLE_FIRST_IRQ;
	TimerIsr*      self = s_instances[irq];
	if (self == nullptr) {
		return;
	}
	// self->timer_->intr = 1u << self->alarm_index;
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

} // namespace naucrates::system

#endif // NAUCRATES_SYSTEM_HARDWARE_TIMER_HPP
