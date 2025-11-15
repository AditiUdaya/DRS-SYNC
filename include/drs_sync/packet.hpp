#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace drs_sync {

enum class PacketType : uint8_t {
    DATA = 0x01,
    ACK = 0x02,
    META = 0x03,
    CHECKPOINT = 0x04,
    RESUME = 0x05
};

enum class Priority : uint8_t {
    NORMAL = 0,
    HIGH = 1,
    CRITICAL = 2
};

enum PacketFlags : uint16_t {
    NONE = 0x0000,
    CHECKPOINT_REQUEST = 0x0001,
    RESUME_REQUEST = 0x0002,
    FINAL_CHUNK = 0x0004,
    INTEGRITY_CHECK = 0x0008
};

#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    Priority priority;
    uint16_t flags;
    uint32_t seq_id;
    uint32_t data_length;
    uint64_t file_size;
    uint64_t file_id;
    uint32_t checksum;
    uint32_t reserved;
    
    PacketHeader() : type(PacketType::DATA), priority(Priority::NORMAL),
                     flags(0), seq_id(0), data_length(0), file_size(0),
                     file_id(0), checksum(0), reserved(0) {}
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 36, "PacketHeader must be 36 bytes");

class Packet {
public:
    static constexpr size_t HEADER_SIZE = sizeof(PacketHeader);
    static constexpr size_t MAX_DATA_SIZE = 65000; // ~64KB
    static constexpr size_t MAX_PACKET_SIZE = HEADER_SIZE + MAX_DATA_SIZE;
    
    PacketHeader header;
    std::vector<uint8_t> data;
    
    Packet() = default;
    
    // Serialize packet to buffer
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer(HEADER_SIZE + data.size());
        std::memcpy(buffer.data(), &header, HEADER_SIZE);
        if (!data.empty()) {
            std::memcpy(buffer.data() + HEADER_SIZE, data.data(), data.size());
        }
        return buffer;
    }
    
    // Deserialize buffer to packet
    static Packet deserialize(const uint8_t* buffer, size_t length) {
        Packet pkt;
        if (length < HEADER_SIZE) return pkt;
        
        std::memcpy(&pkt.header, buffer, HEADER_SIZE);
        
        if (length > HEADER_SIZE) {
            size_t data_len = length - HEADER_SIZE;
            pkt.data.resize(data_len);
            std::memcpy(pkt.data.data(), buffer + HEADER_SIZE, data_len);
        }
        
        return pkt;
    }
    
    size_t total_size() const {
        return HEADER_SIZE + data.size();
    }
};

} // namespace drs_sync