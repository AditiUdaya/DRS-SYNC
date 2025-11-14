// udp_receiver.cpp (WinSock2 version)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed\n";
        return 1;
    }

    std::cout << "[receiver] listening on port 5000...\n";

    char buffer[2048];
    sockaddr_in sender {};
    int sender_len = sizeof(sender);

    while (true) {
        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0,
                             (sockaddr*)&sender, &sender_len);

        if (bytes == SOCKET_ERROR) {
            std::cerr << "recvfrom() failed\n";
            break;
        }

        std::string msg(buffer, buffer + bytes);
        std::cout << "[receiver] got " << bytes << " bytes: " << msg << "\n";
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
