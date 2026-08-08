#ifndef NAUCRATES_RTT_LOGGER_HPP
#define NAUCRATES_RTT_LOGGER_HPP

#include "SEGGER_RTT.h"
#include "etl/span.h"

namespace naucrates {

class RTTLogger {
public:
	static void init() {
		SEGGER_RTT_Init();
	}

	static void write(const char* msg) {
		SEGGER_RTT_WriteString(0, msg);
	}

	template <typename... Args> static void print(const char* fmt, Args... args) {
		SEGGER_RTT_printf(0, fmt, args...);
	}

	static void hex_dump(etl::span<const uint8_t> data, size_t max_bytes = 64) {
		const size_t len = (data.size() < max_bytes) ? data.size() : max_bytes;
		write("  Payload: ");
		for (size_t i = 0; i < len; ++i) {
			print("%02X ", data[i]);
		}
		if (data.size() > len) {
			write("...");
		}
		write("\r\n");
	}
};

} // namespace naucrates

#endif // NAUCRATES_RTT_LOGGER_HPP
