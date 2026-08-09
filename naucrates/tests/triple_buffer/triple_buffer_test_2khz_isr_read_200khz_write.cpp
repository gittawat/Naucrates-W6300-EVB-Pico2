#include <cstdint>

#include "naucrates/utilities/triple_buffer.hpp"
#include "naucrates/utilities/rtt_logger.hpp"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"

using namespace naucrates;

#pragma pack(push, 1)
struct PositionData {
	uint32_t                 counter;
	uint32_t                 checksum;
	uint32_t                 timestamp_us;
	etl::array<uint32_t, 13> _pad{};
};
#pragma pack(pop)

static_assert(sizeof(PositionData) == 64, "PositionData must be 64 bytes");

static uint32_t compute_checksum(const PositionData& d) {
	return d.counter ^ 0xDEADBEEF;
}

static naucrates::TripleBuffer<PositionData> g_buffer;
static etl::atomic<bool>                     g_producer_done{false};
static etl::atomic<bool>                     g_consumer_done{false};

// GPIO 0: producer ISR ticks (100 kHz square wave)
// GPIO 1: consumer read ticks (1 kHz square wave)
static constexpr uint32_t kProducerProbePin = 0;
static constexpr uint32_t kConsumerProbePin = 1;

// 200 kHz ISR producer: one publish every 5 us
static constexpr int64_t  kPublishPeriodUs = 5;
static constexpr uint32_t kPublishCount    = 2010000u; // 10 s at 200 kHz

// 2 kHz ISR consumer: one read every 500 us, timer-paced (start-to-start)
static constexpr int64_t kReadPeriodUs = -500;

// Consumer stats — touched only by core 0 (ISR callback, then main after done)
static uint32_t g_read_count   = 0;
static unsigned g_torn         = 0;
static unsigned g_regressions  = 0;
static uint32_t g_last_counter = 0;
static uint32_t g_max_age_us   = 0;
static uint64_t g_sum_age_us   = 0;
static uint32_t g_age_samples  = 0;
static uint32_t g_max_delta    = 0;
static uint32_t g_min_interval = UINT32_MAX;
static uint32_t g_max_interval = 0;
static uint32_t g_t_prev_read  = 0;

static bool isr_publish_cb(repeating_timer_t* rt) {
	gpio_xor_mask(1u << kProducerProbePin);

	static uint32_t i = 0;
	++i;

	PositionData d{};
	d.counter      = i;
	d.checksum     = compute_checksum(d);
	d.timestamp_us = time_us_32();

	g_buffer.publish(d);

	if (i >= kPublishCount) {
		g_producer_done.store(true, etl::memory_order_release);
		return false; // stop repeating
	}
	return true;
}

static bool consumer_read_cb(repeating_timer_t* rt) {
	gpio_xor_mask(1u << kConsumerProbePin);

	const PositionData& snap   = g_buffer.read_latest();
	uint32_t            t_read = time_us_32();
	++g_read_count;

	// Read-cadence jitter: interval between this tick and the previous one.
	// Skip the first interval — the first callback's t_read is delayed by
	// alarm-pool cold-start bookkeeping, which would skew the min.
	if (g_read_count > 2) {
		uint32_t interval = t_read - g_t_prev_read;
		if (interval < g_min_interval) {
			g_min_interval = interval;
		}
		if (interval > g_max_interval) {
			g_max_interval = interval;
		}
	}
	g_t_prev_read = t_read;

	bool done_now =
	    g_producer_done.load(etl::memory_order_acquire) && snap.counter >= kPublishCount;

	if (snap.counter > 0 && !done_now) {
		if (snap.counter < g_last_counter) {
			++g_regressions;
		}
		if (compute_checksum(snap) != snap.checksum) {
			++g_torn;
		}
		uint32_t age = t_read - snap.timestamp_us;
		if (age > g_max_age_us) {
			g_max_age_us = age;
		}
		g_sum_age_us += age;
		++g_age_samples;
		if (g_last_counter > 0 && snap.counter >= g_last_counter) {
			uint32_t delta = snap.counter - g_last_counter;
			if (delta > g_max_delta) {
				g_max_delta = delta;
			}
		}
	}

	g_last_counter = snap.counter;

	if (done_now) {
		g_consumer_done.store(true, etl::memory_order_release);
		return false; // stop repeating
	}
	return true;
}

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);

	alarm_pool_t*     pool = alarm_pool_create_with_unused_hardware_alarm(2);
	repeating_timer_t rt;
	alarm_pool_add_repeating_timer_us(pool, -kPublishPeriodUs, isr_publish_cb, nullptr, &rt);

	while (!g_producer_done.load(etl::memory_order_acquire)) {
		tight_loop_contents();
	}

	multicore_fifo_push_blocking(0);
}

int main() {
	RTTLogger::init();
	RTTLogger::write("=== TripleBuffer 2kHz ISR Read / 200kHz ISR Write Test ===\r\n");

	gpio_init(kProducerProbePin);
	gpio_set_dir(kProducerProbePin, GPIO_OUT);
	gpio_init(kConsumerProbePin);
	gpio_set_dir(kConsumerProbePin, GPIO_OUT);

	multicore_launch_core1(core1_entry);

	uint32_t handshake = multicore_fifo_pop_blocking();
	static_cast<void>(handshake);

	RTTLogger::write("core1 launched, arming 2 kHz consumer alarm (10 s)\r\n");

	uint32_t t_start = time_us_32();

	repeating_timer_t consumer_timer;
	add_repeating_timer_us(kReadPeriodUs, consumer_read_cb, nullptr, &consumer_timer);

	while (!g_consumer_done.load(etl::memory_order_acquire)) {
		tight_loop_contents();
	}

	cancel_repeating_timer(&consumer_timer);

	uint32_t t_end       = time_us_32();
	uint32_t duration_ms = (t_end - t_start) / 1000u;

	uint32_t producer_done = multicore_fifo_pop_blocking();
	static_cast<void>(producer_done);

	uint32_t publishes      = kPublishCount;
	uint32_t reads_per_sec  = (g_read_count * 1000u) / (duration_ms == 0 ? 1u : duration_ms);
	uint32_t writes_per_sec = (publishes * 1000u) / (duration_ms == 0 ? 1u : duration_ms);
	uint32_t avg_age_us =
	    (g_age_samples == 0) ? 0u : static_cast<uint32_t>(g_sum_age_us / g_age_samples);

	const char* verdict =
	    (g_torn == 0 && g_regressions == 0 && g_last_counter == kPublishCount) ? "PASS" : "FAIL";

	RTTLogger::print("duration:       %lu ms\r\n", static_cast<unsigned long>(duration_ms));
	RTTLogger::print("publishes:      %lu\r\n", static_cast<unsigned long>(publishes));
	RTTLogger::print("consumer reads: %u\r\n", g_read_count);
	RTTLogger::print("read rate:      %u /s\r\n", reads_per_sec);
	RTTLogger::print("write rate:     %u /s\r\n", writes_per_sec);
	RTTLogger::print("last counter:   %lu\r\n", static_cast<unsigned long>(g_last_counter));
	RTTLogger::print("torn reads:     %u\r\n", g_torn);
	RTTLogger::print("regressions:    %u\r\n", g_regressions);
	RTTLogger::print("max data age:           %lu us\r\n",
	                 static_cast<unsigned long>(g_max_age_us));
	RTTLogger::print("avg data age:           %lu us\r\n", static_cast<unsigned long>(avg_age_us));
	RTTLogger::print("max publishes per poll: %u\r\n", g_max_delta);
	RTTLogger::print("poll interval min/max:  %lu / %lu us\r\n",
	                 static_cast<unsigned long>(g_min_interval),
	                 static_cast<unsigned long>(g_max_interval));
	RTTLogger::print("verdict:                %s\r\n", verdict);

	while (true) {
		tight_loop_contents();
	}
}
