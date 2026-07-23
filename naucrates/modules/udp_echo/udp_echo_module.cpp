#include "naucrates/modules/udp_echo/udp_echo_module.hpp"
#include "naucrates/platform/rtt_logger.hpp"
#include "naucrates/platform/interrupt_manager.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "wizchip_spi.h"

#include "etl/memory.h"

extern "C" void wiznet_gpio_isr();

namespace naucrates
{

UdpEchoModule::UdpEchoModule(drivers::wiznet::W6300Driver& wiz,
                             SharedData& data,
                             const UdpEchoConfig& cfg)
    : wiz_(wiz)
    , data_(data)
    , cfg_(cfg)
{
}

void UdpEchoModule::configure()
{
    RTTLogger::write("Configuring UdpEchoModule...\r\n");

    wiz_.enable_socket_interrupt(cfg_.socket_id,
        (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT));
    wiz_.enable_chip_interrupt(1u << cfg_.socket_id);

    irq_delegate_ = etl::delegate<void(size_t)>::create<
        UdpEchoModule, &UdpEchoModule::handle_interrupt>(*this);

    InterruptManagerSingleton::instance()
        .register_handler<IrqId::WiznetInt>(irq_delegate_);

    gpio_add_raw_irq_handler(PIN_INT, &wiznet_gpio_isr);
    gpio_set_irq_enabled(PIN_INT, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    gpio_init(cfg_.led_pin);
    gpio_set_dir(cfg_.led_pin, GPIO_OUT);
    gpio_put(cfg_.led_pin, 0);
}

void UdpEchoModule::handle_interrupt(size_t)
{
    irq_pending_.store(true, etl::memory_order_release);
}

void UdpEchoModule::update()
{
    static uint32_t loop_count = 0;
    ++loop_count;

    if (irq_pending_.exchange(false, etl::memory_order_acq_rel) ||
        wiz_.rx_available(cfg_.socket_id))
    {
        wiz_.clear_rx_interrupt(cfg_.socket_id);
        process_rx_packet();
    }

    if (loop_count % 500000 == 0)
    {
        print_diagnostics(loop_count);
        pulse_heartbeat();
    }

    sleep_us(10);
}

void UdpEchoModule::process_rx_packet()
{
    uint8_t  remote_ip[4]  = {};
    uint16_t remote_port   = 0;
    uint8_t  addr_len      = 4;

    const int32_t rx_bytes = wiz_.recv_from(cfg_.socket_id,
        rx_buf_.data(), static_cast<uint16_t>(rx_buf_.size()),
        remote_ip, &remote_port, &addr_len);

    if (rx_bytes <= 0) return;

    const uint16_t copy_len = (static_cast<uint16_t>(rx_bytes) <= kCommBufferSize)
        ? static_cast<uint16_t>(rx_bytes)
        : static_cast<uint16_t>(kCommBufferSize);
    etl::mem_copy(rx_buf_.data(), copy_len, data_.rx.raw);
    data_.rx.length = copy_len;

    etl::mem_copy(data_.rx.raw, copy_len, data_.tx.raw);
    data_.tx.length = copy_len;

    stats_.packets_received++;
    stats_.bytes_received += static_cast<uint32_t>(rx_bytes);
    size_history_.push(static_cast<uint16_t>(rx_bytes));

    RTTLogger::print("[#%lu] RECV %ld bytes <- %d.%d.%d.%d:%d\r\n",
        stats_.packets_received, rx_bytes,
        remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);
    RTTLogger::hex_dump(etl::span<const uint8_t>(rx_buf_.data(), static_cast<size_t>(rx_bytes)));

    const uint16_t tx_len = static_cast<uint16_t>(rx_bytes);
    const int32_t tx_bytes = wiz_.send_to(cfg_.socket_id,
        rx_buf_.data(), tx_len, remote_ip, remote_port, addr_len);

    if (tx_bytes == rx_bytes)
    {
        stats_.packets_echoed++;
        RTTLogger::print("  [ECHO OK] %ld bytes sent -> %d.%d.%d.%d:%d\r\n",
            tx_bytes, remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3], remote_port);
    }
    else
    {
        stats_.echo_errors++;
        RTTLogger::print("  [ECHO ERR] sent %ld / %ld bytes\r\n", tx_bytes, rx_bytes);
    }

    toggle_led();
}

void UdpEchoModule::toggle_led()
{
    led_state_ = !led_state_;
    gpio_put(cfg_.led_pin, led_state_);
}

void UdpEchoModule::pulse_heartbeat()
{
    gpio_put(cfg_.led_pin, 1);
    sleep_ms(2);
    gpio_put(cfg_.led_pin, led_state_ ? 1 : 0);
}

void UdpEchoModule::print_diagnostics(uint32_t loop_count)
{
    RTTLogger::print("[STATS] rx_pkts=%lu  rx_bytes=%lu  echoed=%lu  errs=%lu  hist_sz=%zu  cycles=%lu\r\n",
        stats_.packets_received, stats_.bytes_received,
        stats_.packets_echoed, stats_.echo_errors,
        size_history_.size(), loop_count);
}

} // namespace naucrates
