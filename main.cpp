#include "pico/stdlib.h"
#include "SEGGER_RTT.h"
#include "etl/array.h"
#include "etl/vector.h"

#include "wizchip_conf.h"
#include "socket.h"
#include "wizchip_spi.h"
#include "wizchip_gpio_irq.h"

constexpr uint8_t  SOCKET_ID  = 0;
constexpr uint16_t UDP_PORT   = 27181;
constexpr uint      LED_PIN    = 25;
constexpr uint16_t RX_BUF_SIZE = 2048;

etl::array<uint8_t, RX_BUF_SIZE> rx_buf;
etl::vector<uint16_t, 10>        packet_size_history;

struct EchoServerStats
{
    uint32_t total_packets = 0;
    uint32_t total_bytes   = 0;
};
EchoServerStats stats;

volatile bool g_udp_irq_flag = false;

void on_wizchip_interrupt()
{
    g_udp_irq_flag = true;
}

int main()
{
    SEGGER_RTT_Init();
    SEGGER_RTT_WriteString(0, "=== W6300-EVB-Pico2  UDP Echo Server ===\r\n");
    SEGGER_RTT_WriteString(0, "System booting...\r\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    SEGGER_RTT_WriteString(0, "W6300: critical section init...\r\n");
    wizchip_cris_initialize();

    SEGGER_RTT_WriteString(0, "W6300: SPI/PIO init...\r\n");
    wizchip_spi_initialize();

    SEGGER_RTT_WriteString(0, "W6300: hardware reset...\r\n");
    wizchip_reset();

    SEGGER_RTT_WriteString(0, "W6300: chip init...\r\n");
    wizchip_initialize();

    SEGGER_RTT_WriteString(0, "W6300: verifying chip ID...\r\n");
    wizchip_check();
    SEGGER_RTT_WriteString(0, "W6300: CIDR=0x6300 verified\r\n");

    wiz_NetInfo net_info = {
        .mac    = {0x00, 0x08, 0xDC, 0x63, 0x00, 0x10},
        .ip     = {10, 10, 10, 10},
        .sn     = {255, 255, 255, 0},
        .gw     = {10, 10, 10, 1},
        .dns    = {8, 8, 8, 8},
        .ipmode = NETINFO_STATIC_V4,
        .dhcp   = NETINFO_STATIC,
    };

    SEGGER_RTT_WriteString(0, "Applying network configuration:\r\n");
    SEGGER_RTT_printf(0, "  IP      : %d.%d.%d.%d\r\n",
                      net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
    SEGGER_RTT_printf(0, "  Netmask : %d.%d.%d.%d\r\n",
                      net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
    SEGGER_RTT_printf(0, "  Gateway : %d.%d.%d.%d\r\n",
                      net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
    SEGGER_RTT_printf(0, "  MAC     : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                      net_info.mac[0], net_info.mac[1], net_info.mac[2],
                      net_info.mac[3], net_info.mac[4], net_info.mac[5]);

    network_initialize(net_info);
    print_network_information(net_info);

    SEGGER_RTT_printf(0, "Opening UDP socket %d on port %d...\r\n", SOCKET_ID, UDP_PORT);
    int8_t sock_ret = socket(SOCKET_ID, Sn_MR_UDP, UDP_PORT, 0x00);
    if (sock_ret != SOCKET_ID)
    {
        SEGGER_RTT_printf(0, "ERROR: socket() returned %d (expected %d)\r\n", sock_ret, SOCKET_ID);
        while (true) { tight_loop_contents(); }
    }
    SEGGER_RTT_WriteString(0, "UDP socket opened (Sn_MR_UDP)\r\n");

    SEGGER_RTT_WriteString(0, "Configuring GPIO 15 interrupt...\r\n");
    wizchip_gpio_interrupt_initialize(SOCKET_ID, on_wizchip_interrupt);
    SEGGER_RTT_WriteString(0, "Echo server ready. Waiting for packets...\r\n\r\n");

    bool   led_state  = false;
    uint32_t loop_count = 0;

    while (true)
    {
        if (g_udp_irq_flag || getSn_RX_RSR(SOCKET_ID) > 0)
        {
            g_udp_irq_flag = false;

            setSn_IR(SOCKET_ID, Sn_IR_RECV);

            uint8_t  remote_ip[4] = {};
            uint16_t remote_port  = 0;
            uint8_t  addrlen      = 4;

            int32_t bytes_received = recvfrom(SOCKET_ID, rx_buf.data(),
                                              static_cast<uint16_t>(rx_buf.size()),
                                              remote_ip, &remote_port, &addrlen);

            if (bytes_received > 0)
            {
                stats.total_packets++;
                stats.total_bytes += static_cast<uint32_t>(bytes_received);

                if (!packet_size_history.full())
                {
                    packet_size_history.push_back(static_cast<uint16_t>(bytes_received));
                }

                SEGGER_RTT_printf(0, "[#%lu] RECV %ld bytes  <-  %d.%d.%d.%d:%d\r\n",
                                  stats.total_packets,
                                  bytes_received,
                                  remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
                                  remote_port);

                uint16_t preview_len = (static_cast<uint16_t>(bytes_received) < 64)
                                           ? static_cast<uint16_t>(bytes_received)
                                           : 64;
                SEGGER_RTT_WriteString(0, "  Payload: ");
                for (uint16_t i = 0; i < preview_len; i++)
                {
                    SEGGER_RTT_printf(0, "%02X ", rx_buf[i]);
                }
                if (static_cast<uint16_t>(bytes_received) > 64)
                {
                    SEGGER_RTT_WriteString(0, "...");
                }
                SEGGER_RTT_WriteString(0, "\r\n");

                int32_t bytes_sent = sendto(SOCKET_ID, rx_buf.data(),
                                            static_cast<uint16_t>(bytes_received),
                                            remote_ip, remote_port, addrlen);

                if (bytes_sent == bytes_received)
                {
                    SEGGER_RTT_printf(0, "  [ECHO OK]  %ld bytes sent  ->  %d.%d.%d.%d:%d\r\n",
                                      bytes_sent,
                                      remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
                                      remote_port);
                }
                else
                {
                    SEGGER_RTT_printf(0, "  [ECHO ERR] sent %ld / %ld bytes\r\n",
                                      bytes_sent, bytes_received);
                }

                led_state = !led_state;
                gpio_put(LED_PIN, led_state);
            }
        }

        loop_count++;
        if (loop_count % 500000 == 0)
        {
            SEGGER_RTT_printf(0, "[STATS] pkts=%lu  bytes=%lu  hist_sz=%d  cycles=%lu\r\n",
                              stats.total_packets,
                              stats.total_bytes,
                              static_cast<int>(packet_size_history.size()),
                              loop_count);

            gpio_put(LED_PIN, 1);
            sleep_ms(2);
            gpio_put(LED_PIN, led_state ? 1 : 0);
        }
    }
}
