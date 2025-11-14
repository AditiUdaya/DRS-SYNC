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
constexpr uint16_t ACK_PORT = 5001;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Usage: udp_sender_reliable <file>\n";
        return 0;
    }

    std::string filename = argv[1];

    // ---- Read file ----
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "[sender] Cannot open file\n";
        return 1;
    }

    in.seekg(0, std::ios::end);
    uint64_t filesize = (uint64_t)in.tellg();
    in.seekg(0);
    in.close();

    std::cout << "[sender] starting, file=" << filename
              << " size=" << filesize << "\n";

    // ---- Winsock init ----
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // ---- Sending socket ----
    SOCKET sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_send == INVALID_SOCKET) {
        std::cerr << "send socket failed, WSA=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // ---- ACK socket ----
    SOCKET sock_ack = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ack == INVALID_SOCKET) {
        std::cerr << "ack socket failed, WSA=" << WSAGetLastError() << "\n";
        closesocket(sock_send);
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(sock_ack, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    setsockopt(sock_ack, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char*)&opt, sizeof(opt));

    sockaddr_in ack_addr{};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    if (bind(sock_ack, (sockaddr*)&ack_addr, sizeof(ack_addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed for ACK socket. WSA=" << WSAGetLastError() << "\n";
        closesocket(sock_send);
        closesocket(sock_ack);
        WSACleanup();
        return 1;
    }

    std::cout << "[sender] ACK socket bind OK on 5001\n";

    // ---- Receiver address ----
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(RECEIVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    // ---- Build META payload ----
    std::string meta_payload = filename + "\n" + std::to_string(filesize);

    PacketHeader h;
    pack_header(h, 10 /* META */, 0, (uint32_t)meta_payload.size());

    std::vector<char> packet(sizeof(PacketHeader) + meta_payload.size());
    memcpy(packet.data(), &h, sizeof(PacketHeader));
    memcpy(packet.data() + sizeof(PacketHeader),
           meta_payload.data(), meta_payload.size());

    // ---- Send META ----
    int sent = sendto(sock_send, packet.data(), (int)packet.size(), 0,
                      (sockaddr*)&dest, sizeof(dest));

    if (sent == SOCKET_ERROR) {
        std::cerr << "[sender] sendto failed WSA=" << WSAGetLastError() << "\n";
        closesocket(sock_send);
        closesocket(sock_ack);
        WSACleanup();
        return 1;
    }

    std::cout << "[sender] META sent: file=" << filename
              << " size=" << filesize
              << " bytes\n";

    closesocket(sock_send);
    closesocket(sock_ack);
    WSACleanup();
    return 0;
}
