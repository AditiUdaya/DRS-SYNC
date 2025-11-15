#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr uint16_t RECEIVER_PORT = 5000;
constexpr uint16_t ACK_PORT = 5001; // optional local socket for ACKs

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: udp_sender_reliable <file>\n";
        return 1;
    }

    std::string path = argv[1];

    // Extract filename only (no path) for META
    std::string::size_type pos = path.find_last_of("/\\");
    std::string filename = (pos == std::string::npos) ? path : path.substr(pos + 1);

    // Read file size (we don't need the file contents for META)
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "[sender] Cannot open file: " << path << "\n";
        return 1;
    }
    in.seekg(0, std::ios::end);
    uint64_t filesize = static_cast<uint64_t>(in.tellg());
    in.close();

    uint32_t chunk_size = 65536;
    uint32_t total_chunks = (filesize + chunk_size - 1) / chunk_size;

    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "[sender] WSAStartup failed\n";
        return 1;
    }

    // Sending socket
    SOCKET sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_send == INVALID_SOCKET) {
        std::cerr << "[sender] socket() failed, WSA=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Optional: local ACK socket (bind) so receiver can reply
    SOCKET sock_ack = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ack == INVALID_SOCKET) {
        std::cerr << "[sender] ack socket failed, WSA=" << WSAGetLastError() << "\n";
        closesocket(sock_send);
        WSACleanup();
        return 1;
    }

    // set reuse options to reduce 'address in use' problems
    BOOL opt = TRUE;
    setsockopt(sock_ack, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    setsockopt(sock_ack, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt));

    sockaddr_in ack_addr{};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    if (bind(sock_ack, (sockaddr*)&ack_addr, sizeof(ack_addr)) == SOCKET_ERROR) {
        std::cerr << "[sender] bind failed for ACK socket. WSA error = " << WSAGetLastError() << "\n";
        closesocket(sock_send);
        closesocket(sock_ack);
        WSACleanup();
        return 1;
    }

    // Receiver address
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    // Build META payload: filename\nfilesize\ntotal_chunks
    std::string meta = filename + "\n" + std::to_string(filesize) + "\n" + std::to_string(total_chunks);

    // Pack header (type 10 = META)
    PacketHeader h;
    pack_header(h, 10, 0, static_cast<uint32_t>(meta.size()));

    // Build packet (header + payload)
    std::vector<char> packet(sizeof(PacketHeader) + meta.size());
    memcpy(packet.data(), &h, sizeof(PacketHeader));
    memcpy(packet.data() + sizeof(PacketHeader), meta.data(), meta.size());

    // Send META
    int sent = sendto(sock_send, packet.data(), static_cast<int>(packet.size()), 0,
                      (sockaddr*)&dest, sizeof(dest));
    if (sent == SOCKET_ERROR) {
        std::cerr << "[sender] sendto META failed, WSA=" << WSAGetLastError() << "\n";
        closesocket(sock_send);
        closesocket(sock_ack);
        WSACleanup();
        return 1;
    }

    std::cout << "[sender] META sent: file=" << filename
              << " size=" << filesize << " bytes"
              << " chunks=" << total_chunks << "\n";

    // Clean up
    closesocket(sock_send);
    closesocket(sock_ack);
    WSACleanup();
    return 0;
}
