#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t type;      // 1=data, 2=ACK, 3=META
    uint32_t id;       // chunk index
    uint32_t length;   // data length
    uint64_t file_size;
};
#pragma pack(pop)
