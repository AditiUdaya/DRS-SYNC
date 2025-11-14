#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr uint16_t LISTEN_PORT = 5000;
constexpr int RECV_BUF = 128 * 1024;

int main() {
    // --- Winsock init ---
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed WSA=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(LISTEN_PORT);
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed WSA=" << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[receiver] listening on port " << LISTEN_PORT << "\n";

    std::vector<char> buf(RECV_BUF);

    while (true) {
        sockaddr_in from{};
        int fromlen = sizeof(from);

        int r = recvfrom(sock, buf.data(), (int)buf.size(), 0,
                         (sockaddr*)&from, &fromlen);

        if (r == SOCKET_ERROR) {
            std::cerr << "recvfrom failed WSA=" << WSAGetLastError() << "\n";
            continue;
        }

        if (r < (int)sizeof(PacketHeader)) {
            std::cerr << "[receiver] packet too small (" << r << " bytes)\n";
            continue;
        }

        PacketHeader h;
        memcpy(&h, buf.data(), sizeof(PacketHeader));

        uint8_t type;
        uint32_t id;
        uint32_t len;
        unpack_header(h, type, id, len);

        // META packet
        if (type == 10) {
            if (r < (int)(sizeof(PacketHeader) + len)) {
                std::cerr << "[receiver] incomplete META payload\n";
                continue;
            }

            std::string payload(buf.data() + sizeof(PacketHeader), len);
            auto pos = payload.find('\n');
            if (pos == std::string::npos) {
                std::cerr << "[receiver] malformed META\n";
                continue;
            }

            std::string filename = payload.substr(0, pos);
            std::string size_str = payload.substr(pos + 1);

            uint64_t filesize = 0;
            try {
                filesize = std::stoull(size_str);
            } catch (...) {
                filesize = 0;
            }

            uint32_t total_chunks =
                (filesize + 65536 - 1) / 65536;

            std::cout << "[receiver] META: file=" << filename
                      << " size=" << filesize
                      << " chunks=" << total_chunks << "\n";
        }
        else {
            std::cout << "[receiver] unknown type=" << (int)type << "\n";
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
