#include <cstring>

#include "pico/stdlib.h"
#include "SEGGER_RTT.h"
#include "etl/array.h"

#include "wizchip_conf.h"
#include "wizchip_spi.h"
#include "socket.h"
#include "wizchip_gpio_irq.h"

constexpr uint LED_PIN        = 25;
constexpr uint8_t SOCK_ECHO   = 0;
constexpr uint16_t ECHO_PORT  = 27181;
constexpr size_t  BUF_SIZE    = 2048;

static volatile bool                g_irq_fired = false;
static uint32_t                     g_pkt_count = 0;
static etl::array<uint8_t, BUF_SIZE> g_buf{};

static wiz_NetInfo g_net_info = {
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56},
    .ip  = {10, 10, 10, 10},
    .sn  = {255, 255, 255, 0},
    .gw  = {10, 10, 10, 1},
    .dns = {10, 10, 10, 1},
    .ipmode = NETINFO_STATIC_ALL,
};

static void echo_irq_callback(void) {
    g_irq_fired = true;
}

int main() {
    SEGGER_RTT_Init();
    SEGGER_RTT_WriteString(0, "=== W6300 UDP Echo (ISR) ===\r\n");

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /* ---- Hardware / network init ---- */
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    network_initialize(g_net_info);

    /* Wait for PHY link */
    {
        uint8_t phy = PHY_LINK_OFF;
        for (int i = 0; i < 50; i++) {
            if (ctlwizchip(CW_GET_PHYLINK, &phy) != -1 && phy == PHY_LINK_ON)
                break;
            sleep_ms(100);
        }
        SEGGER_RTT_printf(0, "PHY link %s\r\n",
                          (phy == PHY_LINK_ON) ? "up" : "DOWN (no cable?)");
    }

    print_network_information(g_net_info);

    /* ---- GP15 interrupt ---- */
    wizchip_gpio_interrupt_initialize(SOCK_ECHO, echo_irq_callback);
    SEGGER_RTT_WriteString(0, "GP15 interrupt enabled\r\n");

    /* ---- Socket ---- */
    uint8_t  dest_ip[4]  = {};
    uint16_t dest_port   = 0;
    uint8_t  addr_len    = 4;
    bool     socket_up   = false;

    SEGGER_RTT_printf(0, "Listening UDP:%u\r\n", ECHO_PORT);

    while (true) {
        /* ---- Socket state machine ---- */
        uint8_t status;
        getsockopt(SOCK_ECHO, SO_STATUS, &status);

        switch (status) {
        case SOCK_UDP:
            if (!socket_up) {
                SEGGER_RTT_WriteString(0, "UDP socket up\r\n");
                socket_up = true;
            }
            break;

        case SOCK_CLOSED:
        case SOCK_INIT:
            if (socket_up) {
                SEGGER_RTT_WriteString(0, "socket closed, re-opening\r\n");
                socket_up = false;
            }
            socket(SOCK_ECHO, Sn_MR_UDP4, ECHO_PORT, 0);
            continue;

        default:
            continue;
        }

        /* ---- Wait for data (interrupt or poll fallback) ---- */
        uint16_t rxsize;
        getsockopt(SOCK_ECHO, SO_RECVBUF, &rxsize);
        if (rxsize == 0 && !g_irq_fired)
            continue;
        g_irq_fired = false;

        /* ---- Drain pending datagrams ---- */
        while (true) {
            getsockopt(SOCK_ECHO, SO_RECVBUF, &rxsize);
            if (rxsize == 0) break;
            if (rxsize > g_buf.size()) rxsize = static_cast<uint16_t>(g_buf.size());

            addr_len = sizeof(dest_ip);
            int32_t n = recvfrom(SOCK_ECHO, g_buf.data(), rxsize, dest_ip, &dest_port, &addr_len);
            if (n <= 0) break;

            g_pkt_count++;
            gpio_put(LED_PIN, 1);

            int32_t sent = sendto(SOCK_ECHO, g_buf.data(), static_cast<uint16_t>(n),
                                  dest_ip, dest_port, addr_len);

            SEGGER_RTT_printf(0, "[%6lu] %ld bytes from %d.%d.%d.%d:%u  echo=%ld\r\n",
                              g_pkt_count, n,
                              dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3],
                              dest_port, sent);

            gpio_put(LED_PIN, 0);
        }
    }
}
