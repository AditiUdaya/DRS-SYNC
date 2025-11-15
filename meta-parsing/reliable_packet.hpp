#ifndef RELIABLE_PACKET_HPP
#define RELIABLE_PACKET_HPP

#include <cstdint>
#include <winsock2.h>   // for htonl / ntohl
#include <ws2tcpip.h>   // for addr conversion helpers

#pragma pack(push,1)
struct PacketHeader {
    uint8_t  type;     // 10 = META
    uint32_t id;       // chunk id (network byte order on wire)
    uint32_t length;   // payload length (network byte order on wire)
};
#pragma pack(pop)

// Pack header to network byte order
inline void pack_header(PacketHeader &h, uint8_t type, uint32_t id, uint32_t length) {
    h.type = type;
    h.id = htonl(id);
    h.length = htonl(length);
}

// Unpack from network byte order to host byte order
inline void unpack_header(const PacketHeader &h, uint8_t &type, uint32_t &id, uint32_t &length) {
    type = h.type;
    id = ntohl(h.id);
    length = ntohl(h.length);
}

#endif // RELIABLE_PACKET_HPP

