#pragma once
#include <cstdint>

#pragma pack(push,1)
struct PacketHeader {
    uint8_t  type;       // 10=META, 1=CHUNK, 2=ACK
    uint32_t chunk_id;
    uint32_t length;
};
#pragma pack(pop)

inline void pack_header(PacketHeader &h, uint8_t type, uint32_t chunk, uint32_t len) {
    h.type = type;
    h.chunk_id = htonl(chunk);
    h.length = htonl(len);
}

inline void unpack_header(PacketHeader &h) {
    h.chunk_id = ntohl(h.chunk_id);
    h.length   = ntohl(h.length);
}
