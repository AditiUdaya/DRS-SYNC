#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "reliable_packet.hpp"

#pragma comment(lib, "Ws2_32.lib")

constexpr uint16_t RECEIVER_PORT = 5000;
constexpr uint16_t ACK_PORT      = 5001;
constexpr uint32_t CHUNK_SIZE    = 65536;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET sock_ack = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed\n";
        return 1;
    }

    std::cout << "[receiver] listening on port 5000\n";

    std::ofstream out("received.dat", std::ios::binary);

    sockaddr_in sender_addr{};
    int sender_len = sizeof(sender_addr);

    while (true) {
        std::vector<char> buf(sizeof(PacketHeader) + CHUNK_SIZE);
        int r = recvfrom(sock, buf.data(), buf.size(), 0,
                         (sockaddr*)&sender_addr, &sender_len);

        if (r <= 0) continue;

        PacketHeader h;
        memcpy(&h, buf.data(), sizeof(h));
        unpack_header(h);

        if (h.type == 1) { // CHUNK
            uint32_t cid = h.chunk_id;
            uint32_t len = h.length;

            out.seekp((uint64_t)cid * CHUNK_SIZE);
            out.write(buf.data() + sizeof(h), len);

            // send ACK
            PacketHeader ack;
            pack_header(ack, 2, cid, 0);

            sendto(sock_ack, (char*)&ack, sizeof(ack), 0,
                   (sockaddr*)&sender_addr, sender_len);

            std::cout << "[receiver] wrote chunk " << cid << "\n";
        }
    }

    return 0;
}
