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

// GPIO 0 toggles on every ISR tick: 100 kHz square wave on a scope
static constexpr uint32_t kIsrProbePin = 0;

// 200 kHz ISR producer: one publish every 5 us
static constexpr int64_t  kPublishPeriodUs = 5;
static constexpr uint32_t kPublishCount    = 2000000u; // 10 s at 200 kHz

// 2 kHz polled consumer: one read every 500 us
static constexpr uint32_t kReadPeriodUs = 499;

static bool isr_publish_cb(repeating_timer_t* rt) {
	gpio_xor_mask(1u << kIsrProbePin);

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
	RTTLogger::write("=== TripleBuffer 2kHz Read / 200kHz ISR Write Test ===\r\n");

	gpio_init(kIsrProbePin);
	gpio_set_dir(kIsrProbePin, GPIO_OUT);

	multicore_launch_core1(core1_entry);

	uint32_t handshake = multicore_fifo_pop_blocking();
	static_cast<void>(handshake);

	RTTLogger::write("core1 launched, beginning 2 kHz consumer loop (10 s)\r\n");

	uint32_t last_counter = 0;
	unsigned torn         = 0;
	unsigned regressions  = 0;
	unsigned read_count   = 0;
	uint32_t max_age_us   = 0;
	uint32_t max_delta    = 0;

	uint32_t t_start = time_us_32();

	while (true) {
		const PositionData& snap   = g_buffer.read_latest();
		uint32_t            t_read = time_us_32();
		++read_count;

		bool done_now =
		    g_producer_done.load(etl::memory_order_acquire) && snap.counter >= kPublishCount;

		if (snap.counter > 0 && !done_now) {
			if (snap.counter < last_counter) {
				++regressions;
			}
			if (compute_checksum(snap) != snap.checksum) {
				++torn;
			}
			uint32_t age = t_read - snap.timestamp_us;
			if (age > max_age_us) {
				max_age_us = age;
			}
			if (last_counter > 0 && snap.counter >= last_counter) {
				uint32_t delta = snap.counter - last_counter;
				if (delta > max_delta) {
					max_delta = delta;
				}
			}
		}

		last_counter = snap.counter;

		if (done_now) {
			break;
		}

		busy_wait_us(kReadPeriodUs);
	}

	uint32_t t_end       = time_us_32();
	uint32_t duration_ms = (t_end - t_start) / 1000u;

	uint32_t producer_done = multicore_fifo_pop_blocking();
	static_cast<void>(producer_done);

	uint32_t publishes      = kPublishCount;
	uint32_t reads_per_sec  = (read_count * 1000u) / (duration_ms == 0 ? 1u : duration_ms);
	uint32_t writes_per_sec = (publishes * 1000u) / (duration_ms == 0 ? 1u : duration_ms);

	const char* verdict =
	    (torn == 0 && regressions == 0 && last_counter == kPublishCount) ? "PASS" : "FAIL";

	RTTLogger::print("duration:       %lu ms\r\n", static_cast<unsigned long>(duration_ms));
	RTTLogger::print("publishes:      %lu\r\n", static_cast<unsigned long>(publishes));
	RTTLogger::print("consumer reads: %u\r\n", read_count);
	RTTLogger::print("read rate:      %u /s\r\n", reads_per_sec);
	RTTLogger::print("write rate:     %u /s\r\n", writes_per_sec);
	RTTLogger::print("last counter:   %lu\r\n", static_cast<unsigned long>(last_counter));
	RTTLogger::print("torn reads:     %u\r\n", torn);
	RTTLogger::print("regressions:    %u\r\n", regressions);
	RTTLogger::print("max data age:           %lu us\r\n", static_cast<unsigned long>(max_age_us));
	RTTLogger::print("max publishes per poll: %u\r\n", max_delta);
	RTTLogger::print("verdict:                %s\r\n", verdict);

	while (true) {
		tight_loop_contents();
	}
}
