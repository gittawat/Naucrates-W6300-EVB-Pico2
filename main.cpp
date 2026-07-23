#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "SEGGER_RTT.h"
#include "etl/array.h"
#include "etl/atomic.h"
#include "etl/circular_buffer.h"
#include "etl/delegate.h"
#include "etl/span.h"

#include "wizchip_conf.h"
#include "socket.h"
#include "wizchip_spi.h"

#include "naucrates/interrupt_manager.hpp"

// ============================================================================
// Configuration
// ============================================================================
struct AppConfig
{
    static constexpr uint8_t  SOCKET_ID   = 0;
    static constexpr uint16_t UDP_PORT    = 27181;
    static constexpr uint     LED_PIN     = 25;
    static constexpr uint16_t RX_BUF_SIZE = 2048;
    static constexpr size_t   HISTORY_CAP = 10;

    static constexpr wiz_NetInfo DEFAULT_NET_INFO = {
        .mac    = {0x00, 0x08, 0xDC, 0x63, 0x00, 0x10},
        .ip     = {10, 10, 10, 10},
        .sn     = {255, 255, 255, 0},
        .gw     = {10, 10, 10, 1},
        .lla    = {},
        .gua    = {},
        .sn6    = {},
        .gw6    = {},
        .dns    = {8, 8, 8, 8},
        .dns6   = {},
        .ipmode = NETINFO_STATIC_V4,
        .dhcp   = NETINFO_STATIC,
    };
};

// ============================================================================
// RTT Logging Utility
// ============================================================================
class RTTLogger
{
public:
    static void init()
    {
        SEGGER_RTT_Init();
    }

    static void write(const char* msg)
    {
        SEGGER_RTT_WriteString(0, msg);
    }

    template <typename... Args>
    static void print(const char* fmt, Args... args)
    {
        SEGGER_RTT_printf(0, fmt, args...);
    }

    static void hex_dump(etl::span<const uint8_t> data, size_t max_bytes = 64)
    {
        const size_t len = (data.size() < max_bytes) ? data.size() : max_bytes;
        write("  Payload: ");
        for (size_t i = 0; i < len; ++i)
        {
            print("%02X ", data[i]);
        }
        if (data.size() > len)
        {
            write("...");
        }
        write("\r\n");
    }
};

// ============================================================================
// ISR Trampolines (extern "C" for Pico SDK compatibility)
// ============================================================================
extern "C" void wiznet_gpio_isr()
{
    naucrates::InterruptManagerSingleton::instance()
        .dispatch<naucrates::IrqId::WiznetInt>();
}

// ============================================================================
// UDP Echo Server Class
// ============================================================================
class UdpEchoServer
{
public:
    struct Statistics
    {
        uint32_t packets_received = 0;
        uint32_t bytes_received   = 0;
        uint32_t packets_echoed   = 0;
        uint32_t echo_errors     = 0;
    };

    UdpEchoServer() = default;

    bool init(const wiz_NetInfo& net_info = AppConfig::DEFAULT_NET_INFO)
    {
        init_led();

        RTTLogger::write("Initializing W6300 hardware...\r\n");
        wizchip_cris_initialize();
        wizchip_spi_initialize();
        wizchip_reset();
        wizchip_initialize();
        wizchip_check();
        RTTLogger::write("W6300 chip hardware verified (CIDR=0x6300)\r\n");

        configure_network(net_info);

        if (!open_socket())
        {
            return false;
        }

        configure_interrupt();
        RTTLogger::write("UDP Echo Server initialized successfully.\r\n\r\n");
        return true;
    }

    void run()
    {
        uint32_t loop_count = 0;

        while (true)
        {
            if (m_irq_pending.exchange(false, etl::memory_order_acq_rel) ||
                getSn_RX_RSR(AppConfig::SOCKET_ID) > 0)
            {
                setSn_IR(AppConfig::SOCKET_ID, Sn_IR_RECV);
                process_rx_packet();
            }

            loop_count++;
            if (loop_count % 500000 == 0)
            {
                print_diagnostics(loop_count);
                pulse_heartbeat();
            }

            sleep_us(10);
        }
    }

private:
    static constexpr uint kIrqPin = PIN_INT;

    etl::atomic<bool>      m_irq_pending{false};
    etl::delegate<void(size_t)> m_irq_delegate{};

    void on_interrupt(size_t id)
    {
        (void)id;
        m_irq_pending.store(true, etl::memory_order_release);
    }

    void init_led()
    {
        gpio_init(AppConfig::LED_PIN);
        gpio_set_dir(AppConfig::LED_PIN, GPIO_OUT);
        gpio_put(AppConfig::LED_PIN, 0);
    }

    void configure_network(const wiz_NetInfo& net_info)
    {
        RTTLogger::write("Applying network configuration:\r\n");
        RTTLogger::print("  IP      : %d.%d.%d.%d\r\n",
                         net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
        RTTLogger::print("  Netmask : %d.%d.%d.%d\r\n",
                         net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
        RTTLogger::print("  Gateway : %d.%d.%d.%d\r\n",
                         net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
        RTTLogger::print("  MAC     : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                         net_info.mac[0], net_info.mac[1], net_info.mac[2],
                         net_info.mac[3], net_info.mac[4], net_info.mac[5]);

        network_initialize(const_cast<wiz_NetInfo&>(net_info));
        print_network_information(const_cast<wiz_NetInfo&>(net_info));
    }

    bool open_socket()
    {
        RTTLogger::print("Opening UDP socket %d on port %d...\r\n",
                         AppConfig::SOCKET_ID, AppConfig::UDP_PORT);

        const int8_t ret = socket(AppConfig::SOCKET_ID, Sn_MR_UDP, AppConfig::UDP_PORT, 0x00);
        if (ret != AppConfig::SOCKET_ID)
        {
            RTTLogger::print("ERROR: socket() failed with code %d\r\n", ret);
            return false;
        }

        RTTLogger::write("UDP socket opened in Sn_MR_UDP mode.\r\n");
        return true;
    }

    void configure_interrupt()
    {
        RTTLogger::write("Configuring W6300 interrupt on GPIO 15...\r\n");

        uint32_t sn_mask = (SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED | SIK_TIMEOUT);
        ctlsocket(AppConfig::SOCKET_ID, CS_SET_INTMASK, &sn_mask);

        uint32_t simr = ((1u << AppConfig::SOCKET_ID) << 8);
        ctlwizchip(CW_SET_INTRMASK, &simr);

        m_irq_delegate = etl::delegate<void(size_t)>::create<
            UdpEchoServer, &UdpEchoServer::on_interrupt>(*this);

        naucrates::InterruptManagerSingleton::instance()
            .register_handler<naucrates::IrqId::WiznetInt>(m_irq_delegate);

        gpio_add_raw_irq_handler(kIrqPin, &wiznet_gpio_isr);
        gpio_set_irq_enabled(kIrqPin, GPIO_IRQ_EDGE_FALL, true);
        irq_set_enabled(IO_IRQ_BANK0, true);
    }

    void process_rx_packet()
    {
        uint8_t  remote_ip[4] = {};
        uint16_t remote_port  = 0;
        uint8_t  addr_len     = 4;

        const int32_t rx_bytes = recvfrom(AppConfig::SOCKET_ID,
                                          m_rx_buf.data(),
                                          static_cast<uint16_t>(m_rx_buf.size()),
                                          remote_ip, &remote_port, &addr_len);

        if (rx_bytes <= 0)
        {
            return;
        }

        const auto bytes_u16 = static_cast<uint16_t>(rx_bytes);

        m_stats.packets_received++;
        m_stats.bytes_received += static_cast<uint32_t>(rx_bytes);
        m_size_history.push(bytes_u16);

        RTTLogger::print("[#%lu] RECV %ld bytes <- %d.%d.%d.%d:%d\r\n",
                         m_stats.packets_received, rx_bytes,
                         remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
                         remote_port);

        RTTLogger::hex_dump(etl::span<const uint8_t>(m_rx_buf.data(), static_cast<size_t>(rx_bytes)));

        const int32_t tx_bytes = sendto(AppConfig::SOCKET_ID,
                                        m_rx_buf.data(),
                                        bytes_u16,
                                        remote_ip, remote_port, addr_len);

        if (tx_bytes == rx_bytes)
        {
            m_stats.packets_echoed++;
            RTTLogger::print("  [ECHO OK] %ld bytes sent -> %d.%d.%d.%d:%d\r\n",
                             tx_bytes,
                             remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
                             remote_port);
        }
        else
        {
            m_stats.echo_errors++;
            RTTLogger::print("  [ECHO ERR] sent %ld / %ld bytes\r\n", tx_bytes, rx_bytes);
        }

        toggle_led();
    }

    void toggle_led()
    {
        m_led_state = !m_led_state;
        gpio_put(AppConfig::LED_PIN, m_led_state);
    }

    void pulse_heartbeat()
    {
        gpio_put(AppConfig::LED_PIN, 1);
        sleep_ms(2);
        gpio_put(AppConfig::LED_PIN, m_led_state ? 1 : 0);
    }

    void print_diagnostics(uint32_t loop_count)
    {
        RTTLogger::print("[STATS] rx_pkts=%lu  rx_bytes=%lu  echoed=%lu  errs=%lu  hist_sz=%zu  cycles=%lu\r\n",
                         m_stats.packets_received,
                         m_stats.bytes_received,
                         m_stats.packets_echoed,
                         m_stats.echo_errors,
                         m_size_history.size(),
                         loop_count);
    }

    etl::array<uint8_t, AppConfig::RX_BUF_SIZE>             m_rx_buf{};
    etl::circular_buffer<uint16_t, AppConfig::HISTORY_CAP> m_size_history{};
    Statistics                                              m_stats{};
    bool                                                    m_led_state = false;
};

// ============================================================================
// Application Entry Point
// ============================================================================
int main()
{
    RTTLogger::init();
    RTTLogger::write("=== W6300-EVB-Pico2 Modern UDP Echo Server ===\r\n");

    naucrates::InterruptManagerSingleton::create();

    UdpEchoServer server;
    if (!server.init())
    {
        RTTLogger::write("FATAL: Server initialization failed!\r\n");
        while (true)
        {
            tight_loop_contents();
        }
    }

    server.run();
}
