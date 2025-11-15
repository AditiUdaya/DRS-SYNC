#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr int RECEIVER_PORT = 5000;
constexpr int ACK_PORT      = 5001;
constexpr uint32_t CHUNK_SIZE = 65536; // 64 KB

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: udp_sender_reliable <file>\n";
        return 1;
    }

    std::string filename = argv[1];

    // Init Winsock
    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);

    SOCKET sock_send = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET sock_ack  = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_send == INVALID_SOCKET || sock_ack == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // ACK socket bind
    sockaddr_in ack_addr {};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    if (bind(sock_ack, (sockaddr*)&ack_addr, sizeof(ack_addr)) == SOCKET_ERROR) {
        std::cerr << "ACK bind failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    std::cout << "[sender] ACK bind OK\n";

    // Read file
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cout << "Cannot open file: " << filename << "\n";
        return 1;
    }

    in.seekg(0, std::ios::end);
    uint64_t filesize = (uint64_t)in.tellg();
    in.seekg(0);

    std::vector<char> filebuf(filesize);
    in.read(filebuf.data(), filesize);
    in.close();

    uint32_t total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    std::cout << "[sender] starting file=" << filename
              << " size=" << filesize
              << " chunks=" << total_chunks << "\n";

    // Receiver addr
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    // ---- SEND META ----
    std::string meta_payload = filename + "\n" + std::to_string(filesize);

    PacketHeader mh{};
    pack_header(mh, 10, 0, (uint32_t)meta_payload.size());

    std::vector<char> meta_packet(sizeof(PacketHeader) + meta_payload.size());
    memcpy(meta_packet.data(), &mh, sizeof(PacketHeader));
    memcpy(meta_packet.data() + sizeof(PacketHeader), meta_payload.data(), meta_payload.size());

    sendto(sock_send, meta_packet.data(), meta_packet.size(), 0,
           (sockaddr*)&dest, sizeof(dest));

    std::cout << "[sender] META sent\n";

    // ---- SEND CHUNKS + WAIT FOR ACKS ----
    sockaddr_in from{};
    int fromlen = sizeof(from);

    for (uint32_t chunk = 0; chunk < total_chunks; chunk++) {
        uint32_t offset = chunk * CHUNK_SIZE;
        uint32_t len = std::min<uint64_t>(CHUNK_SIZE, filesize - offset);

        std::vector<char> packet(sizeof(PacketHeader) + len);
        PacketHeader h{};
        pack_header(h, 1, chunk, len);

        memcpy(packet.data(), &h, sizeof(PacketHeader));
        memcpy(packet.data() + sizeof(PacketHeader),
               filebuf.data() + offset, len);

        bool acked = false;
        while (!acked) {
            sendto(sock_send, packet.data(), packet.size(), 0,
                   (sockaddr*)&dest, sizeof(dest));

            char ackbuf[sizeof(PacketHeader)];
            int r = recvfrom(sock_ack, ackbuf, sizeof(ackbuf), 0,
                             (sockaddr*)&from, &fromlen);

            if (r >= (int)sizeof(PacketHeader)) {
                PacketHeader ah;
                memcpy(&ah, ackbuf, sizeof(PacketHeader));

                uint8_t at = 0;
                uint32_t aid = 0, al = 0;
                unpack_header(ah, at, aid, al);

                if (at == 2 && aid == chunk) {
                    acked = true;
                    break;
                }
            }

            std::cout << "[sender] timeout, retry chunk " << chunk << "\n";
        }
    }

    // ---- DONE ----
    closesocket(sock_send);
    closesocket(sock_ack);
    WSACleanup();
    return 0;
}
