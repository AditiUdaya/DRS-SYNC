#include <iostream>
#include <fstream>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

static const int PORT = 5000;
static const int CHUNK_SIZE = 64000;

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));

    cout << "[receiver] listening on port 5000\n";

    vector<vector<char>> buffer;
    uint64_t file_size = 0;
    int total_chunks = 0;
    int next_write = 0;
    ofstream outfile("received_bigfile.txt", ios::binary);

    sockaddr_in src;
    socklen_t sl = sizeof(src);

    while (true)
    {
        vector<char> pkt(70000);
        int n = recvfrom(sock, pkt.data(), pkt.size(), 0,
                         (sockaddr*)&src, &sl);
        if (n <= 0) continue;

        PacketHeader hdr{};
        memcpy(&hdr, pkt.data(), sizeof(hdr));

        if (hdr.type == 3)
        {
            file_size = hdr.file_size;
            total_chunks = hdr.id;
            buffer.resize(total_chunks);
            cout << "[receiver] META: file_size=" << file_size
                 << " chunks=" << total_chunks << "\n";
            continue;
        }
        else if (hdr.type == 1)
        {
            int id = hdr.id;
            int len = hdr.length;

            if (id < total_chunks)
            {
                buffer[id].assign(pkt.begin() + sizeof(hdr),
                                  pkt.begin() + sizeof(hdr) + len);

                // Send ACK
                PacketHeader ack{};
                ack.type = 2;
                ack.id = id;
                sendto(sock, (char*)&ack, sizeof(ack), 0,
                       (sockaddr*)&src, sizeof(src));

                // Write in order
                while (next_write < total_chunks &&
                       !buffer[next_write].empty())
                {
                    outfile.write(buffer[next_write].data(),
                                  buffer[next_write].size());
                    next_write++;

                    if (next_write == total_chunks)
                    {
                        cout << "[receiver] File reconstructed\n";
                        outfile.close();
                        closesocket(sock);
                        WSACleanup();
                        return 0;
                    }
                }
            }
        }
    }
}
