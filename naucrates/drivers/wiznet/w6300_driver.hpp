#ifndef NAUCRATES_W6300_DRIVER_HPP
#define NAUCRATES_W6300_DRIVER_HPP

#include "socket.h"
#include "wizchip_conf.h"
#include <cstdint>

extern "C" {
#include "wizchip_spi.h"
}

namespace naucrates::drivers::wiznet {

class W6300Driver {
public:
  bool init();

  void configure_network(const wiz_NetInfo &info);

  bool open_udp_socket(uint8_t socket_id, uint16_t port);

  void enable_socket_interrupt(uint8_t socket_id, uint32_t mask);
  void enable_chip_interrupt(uint32_t socket_mask);

  bool rx_available(uint8_t socket_id) const;
  void clear_rx_interrupt(uint8_t socket_id);

  int32_t recv_from(uint8_t socket_id, uint8_t *buf, uint16_t len, uint8_t *ip,
                    uint16_t *port, uint8_t *addr_len);
  int32_t send_to(uint8_t socket_id, const uint8_t *buf, uint16_t len,
                  const uint8_t *ip, uint16_t port, uint8_t addr_len);
};

} // namespace naucrates::drivers::wiznet

#endif // NAUCRATES_W6300_DRIVER_HPP
