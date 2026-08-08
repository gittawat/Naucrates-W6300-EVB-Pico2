#include <cstdint>

#include "naucrates/utilities/triple_buffer.hpp"
#include "naucrates/utilities/rtt_logger.hpp"
#include "pico/stdlib.h"
#include "pico/multicore.h"

using namespace naucrates;

#pragma pack(push, 1)
struct PositionData {
	uint32_t counter;
	uint32_t checksum;
	etl::array<uint32_t,14> _pad{};
};
#pragma pack(pop)

static_assert(sizeof(PositionData) == 64, "PositionData must be 64 bytes");

static uint32_t compute_checksum(const PositionData& d) {
	return d.counter ^ 0xDEADBEEF;
}

static naucrates::TripleBuffer<PositionData> g_buffer;

static constexpr uint32_t kPublishCount = 100000u;

static void core1_entry() {
	multicore_fifo_push_blocking(0xDEADBEEF);

	for (uint32_t i = 1; i <= kPublishCount; ++i) {
		PositionData d{};
		d.counter  = i;
		d.checksum = compute_checksum(d);

		g_buffer.publish(d);
	}

	multicore_fifo_push_blocking(0);
}

int main() {
	RTTLogger::init();
	RTTLogger::write("=== TripleBuffer MCU Test ===\r\n");

	multicore_launch_core1(core1_entry);

	uint32_t handshake = multicore_fifo_pop_blocking();
	static_cast<void>(handshake);

	RTTLogger::write("core1 launched, beginning consumer loop\r\n");

	uint32_t last_counter = 0;
	unsigned torn         = 0;
	unsigned regressions  = 0;
	unsigned read_count   = 0;
	bool     done         = false;

	while (!done) {
		const PositionData& snap = g_buffer.read_latest();
		++read_count;

		uint32_t chk = compute_checksum(snap);

		if (snap.counter > 0) {
			if (snap.counter < last_counter) {
				++regressions;
			}
			if (chk != snap.checksum) {
				++torn;
			}
		}

		last_counter = snap.counter;

		if (snap.counter >= kPublishCount) {
			done = true;
		}
	}

	uint32_t producer_done = multicore_fifo_pop_blocking();
	static_cast<void>(producer_done);

	const char* verdict = (torn == 0 && regressions == 0) ? "PASS" : "FAIL";

	RTTLogger::print("consumer reads: %u\r\n", read_count);
	RTTLogger::print("last counter:   %lu\r\n", static_cast<unsigned long>(last_counter));
	RTTLogger::print("torn reads:     %u\r\n", torn);
	RTTLogger::print("regressions:    %u\r\n", regressions);
	RTTLogger::print("verdict:        %s\r\n", verdict);

	while (true) {
		tight_loop_contents();
	}
}
