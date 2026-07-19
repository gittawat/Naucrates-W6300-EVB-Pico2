#include <cstdint>
#include <cstring>

#include "pico/stdlib.h"
#include "SEGGER_RTT.h"
#include "etl/array.h"

extern "C" {
#include "wizchip_conf.h"
#include "socket.h"
#include "wizchip_spi.h"
#include "wizchip_gpio_irq.h"
}

static constexpr uint8_t  SOCK_ECHO   = 0;
static constexpr uint16_t ECHO_PORT   = 27181;
static constexpr size_t   BUF_SIZE    = 2048;
static constexpr uint     LED_PIN     = 25;
static constexpr uint32_t POLL_PERIOD = 100000;

static volatile bool              g_irq_flag = false;
static etl::array<uint8_t, BUF_SIZE> g_buf{};

extern "C" void udp_isr_callback(void) {
    g_irq_flag = true;
}

int main() {
    SEGGER_RTT_Init();
    SEGGER_RTT_WriteString(0, "UDP Echo Server (ISR) - W6300-EVB-Pico2\r\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    SEGGER_RTT_WriteString(0, "Initializing W6300...\r\n");
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    SEGGER_RTT_WriteString(0, "W6300 initialized.\r\n");

    static wiz_NetInfo g_net_info = {
        .mac    = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56},
        .ip     = {10, 10, 10, 10},
        .sn     = {255, 255, 255, 0},
        .gw     = {10, 10, 10, 1},
        .dns    = {10, 10, 10, 1},
        .ipmode = NETINFO_STATIC_ALL,
    };
    network_initialize(g_net_info);

    SEGGER_RTT_printf(0, "IP: %d.%d.%d.%d  Port: %d\r\n",
                      g_net_info.ip[0], g_net_info.ip[1], g_net_info.ip[2], g_net_info.ip[3],
                      ECHO_PORT);

    wizchip_gpio_interrupt_initialize(SOCK_ECHO, udp_isr_callback);
    SEGGER_RTT_WriteString(0, "GPIO15 IRQ enabled. Waiting for UDP packets...\r\n");

    uint8_t  destip[4]  = {};
    uint16_t destport   = 0;
    uint8_t  addrlen    = 0;
    uint16_t rxsize     = 0;
    uint32_t pkt_count  = 0;
    uint32_t loop_count = 0;

    while (true) {
        if (g_irq_flag || ((++loop_count % POLL_PERIOD) == 0)) {
            g_irq_flag = false;

            uint8_t status;
            getsockopt(SOCK_ECHO, SO_STATUS, &status);

            switch (status) {
                case SOCK_CLOSED:
                case SOCK_INIT:
                    gpio_put(LED_PIN, 0);
                    socket(SOCK_ECHO, Sn_MR_UDP4, ECHO_PORT, 0);
                    SEGGER_RTT_printf(0, "Socket reopened (status=0x%02x)\r\n", status);
                    break;

                case SOCK_UDP:
                    getsockopt(SOCK_ECHO, SO_RECVBUF, &rxsize);
                    while (rxsize > 0) {
                        gpio_put(LED_PIN, 1);

                        if (rxsize > g_buf.size()) rxsize = static_cast<uint16_t>(g_buf.size());
                        addrlen = sizeof(destip);
                        int32_t n = recvfrom(SOCK_ECHO, g_buf.data(), rxsize, destip, &destport, &addrlen);
                        if (n > 0) {
                            pkt_count++;
                            sendto(SOCK_ECHO, g_buf.data(), static_cast<uint16_t>(n), destip, destport, 4);
                            SEGGER_RTT_printf(0, "[%u] %d bytes from %d.%d.%d.%d:%d\r\n",
                                              pkt_count, static_cast<int>(n),
                                              destip[0], destip[1], destip[2], destip[3],
                                              destport);
                        }
                        getsockopt(SOCK_ECHO, SO_RECVBUF, &rxsize);
                    }
                    gpio_put(LED_PIN, 0);
                    break;
            }
        }
    }
}
