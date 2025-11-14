#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

#pragma comment(lib, "ws2_32.lib")

constexpr int ACK_PORT = 40000;
constexpr uint32_t CHUNK_SIZE = 65536;

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cerr << "Usage: udp_sender_reliable <file>\n";
        return 1;
    }

    std::string filename = argv[1];
    std::cout << "[sender] starting, file=" << filename << "\n";

    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file.\n";
        return 1;
    }

    in.seekg(0, std::ios::end);
    uint64_t filesize = (uint64_t)in.tellg();
    in.seekg(0);

    std::vector<char> filebuf(filesize);
    in.read(filebuf.data(), filesize);
    in.close();

    uint32_t total_chunks = (filesize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    std::cout << "[sender] filesize=" << filesize
              << " total_chunks=" << total_chunks << "\n";

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock_ack = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ack == INVALID_SOCKET) {
        std::cerr << "sock_ack create failed WSA=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in ack_addr{};
    ack_addr.sin_family = AF_INET;
    ack_addr.sin_port = htons(ACK_PORT);
    inet_pton(AF_INET, "127.0.0.1", &ack_addr.sin_addr);

    if (bind(sock_ack, (sockaddr*)&ack_addr, sizeof(ack_addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed for ACK socket. WSA error = "
                  << WSAGetLastError() << "\n";
        closesocket(sock_ack);
        WSACleanup();
        return 1;
    }

    std::cout << "[sender] bind OK\n";

    closesocket(sock_ack);
    WSACleanup();

    std::cout << "[sender] file stage complete\n";
    return 0;
}
