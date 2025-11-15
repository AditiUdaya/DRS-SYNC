#pragma once
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  type;      // 10=META, 1=CHUNK, 2=ACK
    uint32_t id;        // chunk index
    uint32_t length;    // payload length
};
#pragma pack(pop)

inline void pack_header(PacketHeader &h, uint8_t type, uint32_t id, uint32_t length) {
    h.type = type;
    h.id = htonl(id);
    h.length = htonl(length);
}

inline void unpack_header(const PacketHeader &h, uint8_t &type, uint32_t &id, uint32_t &length) {
    type = h.type;
    id = ntohl(h.id);
    length = ntohl(h.length);
}
