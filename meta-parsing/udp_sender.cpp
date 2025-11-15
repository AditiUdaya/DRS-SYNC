// udp_sender.cpp (WinSock2 version)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char** argv) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    std::string msg = "hello from sender";
    if (argc >= 2) msg = argv[1];

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&addr, sizeof(addr));

    std::cout << "[sender] sent: " << msg << "\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}
