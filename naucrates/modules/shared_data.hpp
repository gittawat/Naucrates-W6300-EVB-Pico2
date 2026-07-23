#ifndef NAUCRATES_SHARED_DATA_HPP
#define NAUCRATES_SHARED_DATA_HPP

#include <cstdint>
#include <cstddef>

namespace naucrates
{

inline constexpr size_t kCommBufferSize = 68;
inline constexpr size_t kJointCount     = 6;

struct RxData
{
    union {
        uint8_t raw[kCommBufferSize];
        struct {
            int32_t header;
            int32_t joint_freq_cmd[kJointCount];
            uint8_t joint_enable;
            uint32_t outputs;
        };
    };
    uint16_t length{0};
};

struct TxData
{
    union {
        uint8_t raw[kCommBufferSize];
        struct {
            int32_t header;
            int32_t joint_feedback[kJointCount];
            uint32_t inputs;
        };
    };
    uint16_t length{0};
};

struct SharedData
{
    RxData rx{};
    TxData tx{};
};

} // namespace naucrates

#endif // NAUCRATES_SHARED_DATA_HPP
