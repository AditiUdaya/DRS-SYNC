// udp_receiver_reliable.cpp (Reliable UDP Receiver - WinSock)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr uint32_t CHUNK_SIZE = 65536;

int main() {
    //-----------------------------------------------------------
    // WinSock Init
    //-----------------------------------------------------------
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed\n";
        return 1;
    }

    //-----------------------------------------------------------
    // Bind to 0.0.0.0:5000
    //-----------------------------------------------------------
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[receiver] listening on port 5000\n";

    //-----------------------------------------------------------
    // State variables
    //-----------------------------------------------------------
    std::string out_filename = "received.out";
    std::ofstream out;
    uint64_t expected_filesize = 0;
    uint32_t total_chunks = 0;

    std::unordered_map<uint32_t, std::vector<char>> storage;
    uint32_t write_pos = 0;

    //-----------------------------------------------------------
    // MAIN LOOP
    //-----------------------------------------------------------
    while (true) {

        // Buffer for incoming packet
        char buf[70000];
        sockaddr_in from{};
        int fromlen = sizeof(from);

        int r = recvfrom(
            sock,
            buf,
            sizeof(buf),
            0,
            (sockaddr*)&from,
            &fromlen
        );

        if (r == SOCKET_ERROR) {
            std::cerr << "recvfrom error\n";
            break;
        }

        if (r < (int)sizeof(PacketHeader)) {
            continue; // ignore small garbage packet
        }

        //-------------------------------------------------------
        // Parse header
        //-------------------------------------------------------
        PacketHeader ph;
        memcpy(&ph, buf, sizeof(PacketHeader));

        uint8_t  type;
        uint32_t chunk_id;
        uint32_t payload_len;

        unpack_header(ph, type, chunk_id, payload_len);

        const char* payload = buf + sizeof(PacketHeader);

        //-------------------------------------------------------
        // META PACKET
        //-------------------------------------------------------
        if (type == 10) {
            std::string meta(payload, payload + payload_len);
            auto pos = meta.find('\n');

            if (pos != std::string::npos) {
                std::string name = meta.substr(0, pos);
                out_filename = "received_" + name;
                expected_filesize = std::stoull(meta.substr(pos + 1));
                total_chunks = (uint32_t)((expected_filesize + CHUNK_SIZE - 1) / CHUNK_SIZE);

                std::cout << "[receiver] META: file=" << out_filename
                          << ", size=" << expected_filesize
                          << ", chunks=" << total_chunks << "\n";

                out.open(out_filename, std::ios::binary | std::ios::trunc);
            }

            // Send ACK for META
            PacketHeader ackh;
            pack_header(ackh, 1, 0, 0);
            sendto(sock, (const char*)&ackh, sizeof(ackh), 0, (sockaddr*)&from, fromlen);
        }

        //-------------------------------------------------------
        // DATA PACKET
        //-------------------------------------------------------
        else if (type == 0) {
            std::vector<char> chunk(payload, payload + payload_len);
            storage[chunk_id] = std::move(chunk);

            std::cout << "[receiver] chunk " << chunk_id
                      << " (" << payload_len << " bytes)\n";

            // ACK chunk
            PacketHeader ackh;
            pack_header(ackh, 1, chunk_id, 0);
            sendto(sock, (const char*)&ackh, sizeof(ackh), 0, (sockaddr*)&from, fromlen);

            // Try writing in correct order
            while (storage.count(write_pos)) {
                auto &v = storage[write_pos];
                out.seekp((uint64_t)write_pos * CHUNK_SIZE);
                out.write(v.data(), v.size());
                storage.erase(write_pos);
                write_pos++;
            }
        }

        //-------------------------------------------------------
        // FINAL PACKET
        //-------------------------------------------------------
        else if (type == 2) {
            std::cout << "[receiver] FINAL received\n";

            if (out.is_open())
                out.close();

            // Final ACK
            PacketHeader ackh;
            pack_header(ackh, 1, chunk_id, 0);
            sendto(sock, (const char*)&ackh, sizeof(ackh), 0, (sockaddr*)&from, fromlen);

            break; // end transfer
        }

        //-------------------------------------------------------
        // UNKNOWN TYPE
        //-------------------------------------------------------
        else {
            std::cout << "[receiver] unknown type=" << (int)type << "\n";
        }
    }

    closesocket(sock);
    WSACleanup();

    std::cout << "[receiver] done\n";
    return 0;
}
