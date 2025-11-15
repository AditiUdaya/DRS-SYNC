#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr int RECEIVER_PORT = 5000;
constexpr int ACK_PORT = 5001;
constexpr uint32_t CHUNK_SIZE = 65536;

int main() {
    WSADATA w;
    WSAStartup(MAKEWORD(2,2), &w);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKET ack  = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    bind(sock, (sockaddr*)&addr, sizeof(addr));

    sockaddr_in ack_addr{};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    std::cout << "[receiver] listening on port 5000\n";

    std::unordered_map<uint32_t, std::vector<char>> storage;

    std::string outname;
    uint64_t filesize = 0;
    uint32_t total_chunks = 0;

    auto t_start = std::chrono::high_resolution_clock::now();

    while (true) {
        std::vector<char> buf(sizeof(PacketHeader) + CHUNK_SIZE);
        sockaddr_in from{};
        int fromlen = sizeof(from);

        int r = recvfrom(sock, buf.data(), buf.size(), 0,
                         (sockaddr*)&from, &fromlen);
        if (r < (int)sizeof(PacketHeader)) continue;

        PacketHeader h{};
        memcpy(&h, buf.data(), sizeof(PacketHeader));

        uint8_t type = 0;
        uint32_t id = 0, len = 0;
        unpack_header(h, type, id, len);

        // ---- META ----
        if (type == 10) {
            std::string payload(buf.data() + sizeof(PacketHeader), len);

            auto pos = payload.find('\n');
            if (pos == std::string::npos) continue;

            outname = payload.substr(0, pos);
            std::string fs_str = payload.substr(pos + 1);
            filesize = std::stoull(fs_str);
            total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;

            std::cout << "[receiver] META: file=" << outname
                      << " size=" << filesize
                      << " chunks=" << total_chunks << "\n";
            continue;
        }

        // ---- CHUNK ----
        if (type == 1) {
            std::vector<char> chunk(buf.begin() + sizeof(PacketHeader),
                                    buf.begin() + sizeof(PacketHeader) + len);

            storage[id] = chunk;

            std::cout << "[receiver] wrote chunk " << id << "\n";

            // send ACK
            PacketHeader ah{};
            pack_header(ah, 2, id, 0);

            sendto(ack, (char*)&ah, sizeof(ah), 0,
                   (sockaddr*)&from, fromlen);

            // check completion
            if (storage.size() == total_chunks) {
                auto t_end = std::chrono::high_resolution_clock::now();
                double secs = std::chrono::duration<double>(t_end - t_start).count();

                double mb = (double)filesize / (1024.0 * 1024.0);
                double mbps = mb / secs;

                std::cout << "[receiver] ALL chunks received\n";
                std::cout << "[receiver] Benchmark: " << mb << " MB in " << secs
                          << " s => " << mbps << " MB/s\n";

                // write output file
                std::ofstream out("received_" + outname,
                                  std::ios::binary | std::ios::trunc);

                for (uint32_t i = 0; i < total_chunks; i++) {
                    out.write(storage[i].data(), storage[i].size());
                }
                out.close();

                break;
            }
        }
    }

    closesocket(sock);
    closesocket(ack);
    WSACleanup();
    return 0;
}
