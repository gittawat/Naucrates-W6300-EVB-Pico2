#include <cstdint>

#include "naucrates/system/hardware_timer.hpp"
#include "naucrates/utilities/rtt_logger.hpp"
#include "etl/delegate.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "pico/time.h"

using namespace naucrates;

// GPIO 2: ISR tick probe (2 kHz square wave)
static constexpr uint32_t kProbePin      = 2;
static constexpr int64_t  kPeriodUs      = -500; // 2 kHz, start-to-start
static constexpr uint32_t kTicksPerPhase = 2000; // ~1 s at 2 kHz

struct PhaseStats {
	uint32_t ticks        = 0;
	uint32_t min_interval = UINT32_MAX;
	uint32_t max_interval = 0;
	uint32_t t_prev       = 0;
};

static PhaseStats g_stats;

static void reset_stats() {
	g_stats.ticks        = 0;
	g_stats.min_interval = UINT32_MAX;
	g_stats.max_interval = 0;
	g_stats.t_prev       = 0;
}

static void record_tick() {
	gpio_xor_mask(1u << kProbePin);
	++g_stats.ticks;
	uint32_t t = time_us_32();
	if (g_stats.ticks > 1) {
		uint32_t interval = t - g_stats.t_prev;
		if (interval < g_stats.min_interval) {
			g_stats.min_interval = interval;
		}
		if (interval > g_stats.max_interval) {
			g_stats.max_interval = interval;
		}
	}
	g_stats.t_prev = t;
}

// --- Binding style (a): compile-time free function ---
static bool free_fn_handler() {
	record_tick();
	return g_stats.ticks < kTicksPerPhase;
}

// --- Binding style (b): member function ---
class TickHandler {
public:
	bool on_tick() {
		record_tick();
		return g_stats.ticks < kTicksPerPhase;
	}
};

static void print_phase(const char* label) {
	RTTLogger::print("%s: ticks=%u  interval min/max=%lu/%lu us\r\n",
	                 label,
	                 g_stats.ticks,
	                 static_cast<unsigned long>(g_stats.min_interval),
	                 static_cast<unsigned long>(g_stats.max_interval));
}

int main() {
	RTTLogger::init();
	RTTLogger::write("=== ETL Interrupt Delegate Test ===\r\n");

	gpio_init(kProbePin);
	gpio_set_dir(kProbePin, GPIO_OUT);

	system::DefaultHardwareTimer timer;
	if (!timer.init()) {
		RTTLogger::write("FAIL: timer.init() could not create alarm pool\r\n");
		while (true) {
			tight_loop_contents();
		}
	}

	// Phase (a): compile-time free function
	RTTLogger::write("phase (a): compile-time free function\r\n");
	reset_stats();
	timer.start_repeating_us<&free_fn_handler>(kPeriodUs);
	while (timer.running()) {
		tight_loop_contents();
	}
	timer.stop();
	print_phase("  free_fn  ");

	// Phase (b): member function via etl::delegate::create<T, &Method>(obj)
	RTTLogger::write("phase (b): member function binding\r\n");
	reset_stats();
	TickHandler handler;
	auto member_cb =
	    system::DefaultHardwareTimer::RepeatingCallback::create<TickHandler,
	                                                             &TickHandler::on_tick>(
	        handler);
	timer.start_repeating_us(kPeriodUs, member_cb);
	while (timer.running()) {
		tight_loop_contents();
	}
	timer.stop();
	print_phase("  member   ");

	// Phase (c): non-capturing lambda via +[] (decays to function pointer)
	RTTLogger::write("phase (c): non-capturing lambda via +[]\r\n");
	reset_stats();
	timer.start_repeating_us(
	    kPeriodUs, etl::delegate<bool()>(+[]() -> bool {
		    record_tick();
		    return g_stats.ticks < kTicksPerPhase;
	    }));
	while (timer.running()) {
		tight_loop_contents();
	}
	timer.stop();
	print_phase("  lambda   ");

	bool pass = (g_stats.ticks == kTicksPerPhase);
	RTTLogger::print("verdict: %s\r\n", pass ? "PASS" : "FAIL");

	while (true) {
		tight_loop_contents();
	}
}
