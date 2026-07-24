#ifndef NAUCRATES_UDP_ECHO_MODULE_HPP
#define NAUCRATES_UDP_ECHO_MODULE_HPP

#include "etl/array.h"
#include "etl/atomic.h"
#include "etl/circular_buffer.h"
#include "etl/span.h"

#include "naucrates/modules/shared_data.hpp"
#include "naucrates/drivers/wiznet/w6300_driver.hpp"
#include "wizchip_conf.h"

namespace naucrates
{

struct UdpEchoConfig
{
    uint8_t  socket_id;
    uint16_t port;
    uint16_t rx_buf_size;
    size_t   history_cap;
    unsigned int led_pin;
    wiz_NetInfo net_info;
};

class UdpEchoModule
{
public:
    struct Statistics
    {
        uint32_t packets_received = 0;
        uint32_t bytes_received   = 0;
        uint32_t packets_echoed   = 0;
        uint32_t echo_errors      = 0;
    };

    UdpEchoModule(drivers::wiznet::W6300Driver& wiz,
                  SharedData& data,
                  const UdpEchoConfig& cfg);

    UdpEchoModule(const UdpEchoModule&)            = delete;
    UdpEchoModule& operator=(const UdpEchoModule&) = delete;
    UdpEchoModule(UdpEchoModule&&)                 = delete;
    UdpEchoModule& operator=(UdpEchoModule&&)      = delete;

    void configure();
    void update();
    void handle_interrupt();

private:
    drivers::wiznet::W6300Driver& wiz_;
    SharedData&  data_;
    const UdpEchoConfig& cfg_;

    etl::atomic<bool> irq_pending_{false};

    etl::array<uint8_t, 2048> rx_buf_{};
    etl::circular_buffer<uint16_t, 10> size_history_{};
    Statistics stats_{};
    bool led_state_ = false;

    uint32_t next_diag_time_ = 0;
    uint32_t heartbeat_deadline_ = 0;
    bool heartbeat_active_ = false;

    void process_rx_packet();
    void toggle_led();
    void print_diagnostics();
};

} // namespace naucrates

#endif // NAUCRATES_UDP_ECHO_MODULE_HPP
