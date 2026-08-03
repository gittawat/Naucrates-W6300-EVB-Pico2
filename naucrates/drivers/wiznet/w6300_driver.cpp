#include "naucrates/drivers/wiznet/w6300_driver.hpp"
#include "naucrates/utilities/rtt_logger.hpp"

namespace naucrates::drivers::wiznet
{

bool W6300Driver::init()
{
    RTTLogger::write("Initializing W6300 hardware...\r\n");
    wizchip_cris_initialize();
    wizchip_spi_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    RTTLogger::write("W6300 chip hardware verified (CIDR=0x6300)\r\n");
    return true;
}

void W6300Driver::configure_network(const wiz_NetInfo& info)
{
    RTTLogger::write("Applying network configuration:\r\n");
    RTTLogger::print("  IP      : %d.%d.%d.%d\r\n",
        info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
    RTTLogger::print("  Netmask : %d.%d.%d.%d\r\n",
        info.sn[0], info.sn[1], info.sn[2], info.sn[3]);
    RTTLogger::print("  Gateway : %d.%d.%d.%d\r\n",
        info.gw[0], info.gw[1], info.gw[2], info.gw[3]);
    RTTLogger::print("  MAC     : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
        info.mac[0], info.mac[1], info.mac[2],
        info.mac[3], info.mac[4], info.mac[5]);

    network_initialize(const_cast<wiz_NetInfo&>(info));
    print_network_information(const_cast<wiz_NetInfo&>(info));
}

bool W6300Driver::open_udp_socket(uint8_t socket_id, uint16_t port)
{
    RTTLogger::print("Opening UDP socket %d on port %d...\r\n", socket_id, port);
    const int8_t ret = socket(socket_id, Sn_MR_UDP, port, 0x00);
    if (ret != static_cast<int8_t>(socket_id))
    {
        RTTLogger::print("ERROR: socket() failed with code %d\r\n", ret);
        return false;
    }
    RTTLogger::write("UDP socket opened in Sn_MR_UDP mode.\r\n");
    return true;
}

void W6300Driver::enable_socket_interrupt(uint8_t socket_id, uint32_t mask)
{
    ctlsocket(socket_id, CS_SET_INTMASK, &mask);
}

void W6300Driver::enable_chip_interrupt(uint32_t socket_mask)
{
    uint32_t simr = (socket_mask << 8);
    ctlwizchip(CW_SET_INTRMASK, &simr);
}

bool W6300Driver::rx_available(uint8_t socket_id) const
{
    return getSn_RX_RSR(socket_id) > 0;
}

void W6300Driver::clear_rx_interrupt(uint8_t socket_id)
{
    setSn_IR(socket_id, Sn_IR_RECV);
}

int32_t W6300Driver::recv_from(uint8_t socket_id, uint8_t* buf, uint16_t len,
                               uint8_t* ip, uint16_t* port, uint8_t* addr_len)
{
    return recvfrom(socket_id, buf, len, ip, port, addr_len);
}

int32_t W6300Driver::send_to(uint8_t socket_id, const uint8_t* buf, uint16_t len,
                             const uint8_t* ip, uint16_t port, uint8_t addr_len)
{
    return sendto(socket_id, const_cast<uint8_t*>(buf), len,
                  const_cast<uint8_t*>(ip), port, addr_len);
}

} // namespace naucrates::drivers::wiznet
