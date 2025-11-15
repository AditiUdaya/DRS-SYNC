#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include "reliable_packet.hpp"
#include <fstream>
#pragma comment(lib, "ws2_32.lib")

constexpr uint16_t LISTEN_PORT = 5000;
constexpr int RECV_BUF = 256 * 1024;

int main() {
    // Winsock
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) {
        std::cerr << "[receiver] WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[receiver] socket() failed, WSA=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(LISTEN_PORT);
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR) {
        std::cerr << "[receiver] bind failed on port " << LISTEN_PORT
                  << " WSA=" << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[receiver] listening on port " << LISTEN_PORT << "\n";

    std::vector<char> buf(RECV_BUF);

    while (true) {
        sockaddr_in from{};
        int fromlen = sizeof(from);

        int r = recvfrom(sock, buf.data(), static_cast<int>(buf.size()), 0,
                         (sockaddr*)&from, &fromlen);
        if (r == SOCKET_ERROR) {
            std::cerr << "[receiver] recvfrom failed WSA=" << WSAGetLastError() << "\n";
            continue;
        }

        if (r < static_cast<int>(sizeof(PacketHeader))) {
            std::cerr << "[receiver] packet too small: " << r << " bytes\n";
            continue;
        }

        PacketHeader h;
        memcpy(&h, buf.data(), sizeof(PacketHeader));

        uint8_t type;
        uint32_t id, len;
        unpack_header(h, type, id, len);

        if (type == 10) { // META
            if (r < static_cast<int>(sizeof(PacketHeader) + len)) {
                std::cerr << "[receiver] incomplete META payload\n";
                continue;
            }

            std::string payload(buf.data() + sizeof(PacketHeader), len);

            // Expect: filename\nfilesize\ntotal_chunks
            size_t p1 = payload.find('\n');
            if (p1 == std::string::npos) {
                std::cerr << "[receiver] META malformed (no newline)\n";
                continue;
            }
            size_t p2 = payload.find('\n', p1 + 1);
            if (p2 == std::string::npos) {
                std::cerr << "[receiver] META malformed (missing filesize or chunks)\n";
                continue;
            }

            std::string filename = payload.substr(0, p1);
            std::string filesize_str = payload.substr(p1 + 1, p2 - (p1 + 1));
            std::string chunks_str = payload.substr(p2 + 1);

            uint64_t filesize = 0;
            uint32_t total_chunks = 0;
            try {
                filesize = std::stoull(filesize_str);
                total_chunks = static_cast<uint32_t>(std::stoul(chunks_str));
            } catch (...) {
                std::cerr << "[receiver] META parse error\n";
                continue;
            }

            std::cout << "[receiver] META: file=" << filename
                      << " size=" << filesize << " bytes"
                      << " chunks=" << total_chunks << "\n";

            // Prepare file for incoming chunks (create or truncate)
            std::string outname = "received_" + filename;
            // Create/truncate file to expected size (sparse or zero-filled)
            {
                std::ofstream out(outname, std::ios::binary | std::ios::trunc);
                if (!out) {
                    std::cerr << "[receiver] cannot create output file: " << outname << "\n";
                } else {
                    // Optionally pre-allocate (write a zero at last byte) if filesize > 0
                    if (filesize > 0) {
                        out.seekp(static_cast<std::streamoff>(filesize - 1));
                        char z = 0;
                        out.write(&z, 1);
                    }
                }
            }

            // At this point the receiver knows expected chunks and filesize.
            // You can now switch to chunk-receive logic (Phase 4/5).
            // For now we continue listening for further packets (META or chunks).
        } else {
            // For Phase 3 we only expect META; other types can be printed
            std::cout << "[receiver] got packet type=" << (int)type
                      << " id=" << id << " len=" << len << " bytes_recv=" << r << "\n";
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
