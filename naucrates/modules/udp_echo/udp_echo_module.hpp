#ifndef NAUCRATES_UDP_ECHO_MODULE_HPP
#define NAUCRATES_UDP_ECHO_MODULE_HPP

#include "etl/array.h"
#include "etl/atomic.h"
#include "etl/circular_buffer.h"
#include "etl/delegate.h"
#include "etl/span.h"

#include "naucrates/platform/interrupt_handlers.hpp"
#include "naucrates/modules/module.hpp"
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

class UdpEchoModule : public Module
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

    void configure() override;
    void update() override;
    void handle_interrupt() override;

private:
    drivers::wiznet::W6300Driver& wiz_;
    SharedData&  data_;
    const UdpEchoConfig& cfg_;

    etl::atomic<bool> irq_pending_{false};
    irq::IrqCallback irq_delegate_{};

    etl::array<uint8_t, 2048> rx_buf_{};
    etl::circular_buffer<uint16_t, 10> size_history_{};
    Statistics stats_{};
    bool led_state_ = false;

    void process_rx_packet();
    void toggle_led();
    void pulse_heartbeat();
    void print_diagnostics(uint32_t loop_count);
};

} // namespace naucrates

#endif // NAUCRATES_UDP_ECHO_MODULE_HPP
