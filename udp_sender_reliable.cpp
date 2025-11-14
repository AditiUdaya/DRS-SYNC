#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>

#include "reliable_packet.hpp"

#pragma comment(lib, "Ws2_32.lib")

constexpr uint16_t RECEIVER_PORT = 5000;
constexpr uint16_t ACK_PORT      = 5001;
constexpr uint32_t CHUNK_SIZE    = 65536;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: udp_sender_reliable <file>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file.\n";
        return 1;
    }

    in.seekg(0, std::ios::end);
    uint64_t filesize = in.tellg();
    in.seekg(0);

    std::vector<char> filebuf(filesize);
    in.read(filebuf.data(), filesize);
    in.close();

    uint32_t total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    std::cout << "[sender] starting file=" << filename
              << " size=" << filesize
              << " chunks=" << total_chunks << "\n";

    // -----------------------------
    // Winsock init
    // -----------------------------
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET sock_ack = socket(AF_INET, SOCK_DGRAM, 0);

    // ACK socket bind
    sockaddr_in ack_addr{};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    if (bind(sock_ack, (sockaddr*)&ack_addr, sizeof(ack_addr)) == SOCKET_ERROR) {
        std::cerr << "ACK bind failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    std::cout << "[sender] ACK bind OK\n";

    // Receiver address
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    // --------------------------------------------------------
    // SEND CHUNKS
    // --------------------------------------------------------
    std::vector<char> packet(sizeof(PacketHeader) + CHUNK_SIZE);

    for (uint32_t cid = 0; cid < total_chunks; cid++) {
        uint32_t offset = cid * CHUNK_SIZE;
        uint32_t len = std::min<uint64_t>(CHUNK_SIZE, filesize - offset);

        PacketHeader h;
        pack_header(h, 1 /*CHUNK*/, cid, len);

        memcpy(packet.data(), &h, sizeof(h));
        memcpy(packet.data() + sizeof(h), filebuf.data() + offset, len);

        bool acked = false;

        while (!acked) {
            sendto(sock, packet.data(), sizeof(h) + len, 0,
                   (sockaddr*)&dest, sizeof(dest));

            // wait for ACK
            PacketHeader ackh;
            sockaddr_in from{};
            int fromlen = sizeof(from);

            int r = recvfrom(sock_ack, (char*)&ackh, sizeof(ackh), 0,
                             (sockaddr*)&from, &fromlen);

            if (r > 0) {
                unpack_header(ackh);

                if (ackh.type == 2 && ackh.chunk_id == cid) {
                    acked = true;
                    std::cout << "[sender] ACK for chunk " << cid << "\n";
                }
            }
        }
    }

    std::cout << "[sender] ALL CHUNKS SENT OK\n";

    closesocket(sock);
    closesocket(sock_ack);
    WSACleanup();
    return 0;
}
