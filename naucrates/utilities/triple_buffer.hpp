#ifndef NAUCRATES_TRIPLE_BUFFER_HPP
#define NAUCRATES_TRIPLE_BUFFER_HPP

#include <cstdint>
#include <etl/atomic/atomic_std.h>
#include <type_traits>

#include "etl/array.h"
#include "etl/atomic.h"
namespace naucrates {

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/// @brief Lock-free single-producer single-consumer triple buffer.
///
/// The producer hands off data by copying a snapshot into its slot
/// (publish()) and atomically exchanging shared_ so the consumer's
/// next read_latest() sees it.  Three slots guarantee neither side
/// ever blocks — the producer always has a free slot and the consumer
/// always has a stable slot to read from.
///
/// The consumer calls read_latest() to get a const-ref to the most
/// recently published snapshot.  If no new data was published since
/// the last call the buffer is returned unchanged (no swap, zero
/// overhead).  clear() resets the dirty flag for error-recovery.
/// Under nanosecond-scale contention the boolean dirty_ flag can cause
/// the consumer to receive a slot 1-2 publishes stale (the data is
/// never torn or corrupt — only slightly out-of-date).  This does not
/// occur at realistic ISR-to-polled rates.
///
/// @warning  Exactly ONE producer and ONE consumer.  If data needs to
///           be updated from multiple contexts (e.g. ISR writes
///           jointFeedback at 40 kHz while the servo thread writes
///           processVariable at 1 kHz), the multi-writer arbitration
///           belongs in the comms layer — not inside this buffer.

template <TriviallyCopyable T> class TripleBuffer {
	etl::array<T, 3>     buf_{};
	etl::atomic<uint8_t> shared_{1};
	etl::atomic<bool>    dirty_{false};
	uint8_t              p_idx_{0};
	uint8_t              c_idx_{2};

public:
	TripleBuffer() = default;

	// diabling move and copy
	TripleBuffer(const TripleBuffer&)            = delete;
	TripleBuffer& operator=(const TripleBuffer&) = delete;
	TripleBuffer(TripleBuffer&&)                 = delete;
	TripleBuffer& operator=(TripleBuffer&&)      = delete;

	/// Copies src into the producer's exclusive slot, then hands it
	/// off so it becomes visible to the consumer.  The only atomic
	/// operation is the index exchange inside.
	void publish(const T& src) {
		buf_[p_idx_] = src;
		p_idx_       = shared_.exchange(p_idx_, etl::memory_order_release);
		dirty_.store(true, etl::memory_order_release);
	}

	/// Reset the dirty flag.  The next read_latest() will return the
	/// existing consumer slot without swapping.  Useful for watchdog
	/// or error-recovery paths where stale data is safer than garbage.
	void clear() {
		dirty_.store(false, etl::memory_order_release);
	}

	[[nodiscard]] const T& read_latest() {
		if (dirty_.load(etl::memory_order_relaxed) &&
		    dirty_.exchange(false, etl::memory_order_acq_rel)) {
			c_idx_ = shared_.exchange(c_idx_, etl::memory_order_acquire);
		}
		return buf_[c_idx_];
	}
};

} // namespace naucrates

#endif // NAUCRATES_TRIPLE_BUFFER_HPP
