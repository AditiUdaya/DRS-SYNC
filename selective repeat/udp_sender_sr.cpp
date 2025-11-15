// udp_sender_sr.cpp
// Fixed: removed deadlock by not holding mtx while calling send_packet.
// Metrics + SRTT + window util + debug tag included.

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cstring>
#include <fstream>
#include <iomanip>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "reliable_packet.hpp"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

static const int PORT = 5000;
static const int WINDOW_SIZE = 32;
static const int CHUNK_SIZE = 64000;
static const int TIMEOUT_MS = 200;

// Packet struct
struct Packet {
    vector<char> data;
    bool sent = false;
    bool acked = false;
    int send_count = 0;
    chrono::steady_clock::time_point last_sent;
};

vector<Packet> packets;
int total_chunks = 0;

atomic<int> send_base(0);
atomic<int> total_acks(0);

atomic<long long> sent_bytes_total(0);
atomic<long long> retrans_total(0);

double srtt_ms = 0.0;
mutex mtx;

SOCKET sock = INVALID_SOCKET;
sockaddr_in recv_addr;

// Debug header
void debug_print_header() {
    cout << "DEBUG: SENDER VERSION SR-METRICS-2025-FIX\n";
}

// send_packet: locks only inside for updating packet metadata
void send_packet(int id)
{
    if (id < 0 || id >= total_chunks) return;

    PacketHeader hdr{};
    hdr.type = 1;
    hdr.id = static_cast<uint32_t>(id);
    hdr.length = static_cast<uint32_t>(packets[id].data.size());

    vector<char> buf(sizeof(hdr) + hdr.length);
    memcpy(buf.data(), &hdr, sizeof(hdr));
    if (hdr.length > 0)
        memcpy(buf.data() + sizeof(hdr), packets[id].data.data(), hdr.length);

    bool is_retx = false;

    // update packet state under lock
    {
        lock_guard<mutex> lock(mtx);
        if (packets[id].sent) is_retx = true;
        packets[id].sent = true;
        packets[id].send_count++;
        packets[id].last_sent = chrono::steady_clock::now();
    }

    int s = sendto(sock, buf.data(), static_cast<int>(buf.size()), 0,
                   (sockaddr*)&recv_addr, sizeof(recv_addr));

    if (s > 0) {
        sent_bytes_total += s;
        if (is_retx) retrans_total++;
    }

    // Print debug (we print even if sendto failed; helps diagnose)
    if (!is_retx)
        cout << "DEBUG: sent packet id=" << id << " size=" << hdr.length << "\n";
    else
        cout << "DEBUG: retransmitted packet id=" << id << " size=" << hdr.length << "\n";
}

// ack listener: handles incoming ACKs and updates SRTT
void ack_listener()
{
    cout << "DEBUG: ack_listener started\n";
    sockaddr_in src{};
    socklen_t sl = sizeof(src);

    while (total_acks < total_chunks)
    {
        PacketHeader ack{};
        int n = recvfrom(sock, (char*)&ack, sizeof(ack), 0,
                         (sockaddr*)&src, &sl);
        if (n <= 0) continue;
        if (ack.type != 2) continue;

        int id = static_cast<int>(ack.id);
        if (id < 0 || id >= total_chunks) continue;

        chrono::steady_clock::time_point sent_time;
        bool first_time = false;

        {
            lock_guard<mutex> lock(mtx);
            if (!packets[id].acked) {
                packets[id].acked = true;
                total_acks++;
                sent_time = packets[id].last_sent;
                first_time = true;
                // sliding of send_base will be done below (without holding lock)
            }
        }

        if (first_time) {
            cout << "DEBUG: received ACK id=" << id << "\n";

            // compute RTT sample and update srtt_ms
            double rtt_sample = chrono::duration<double, milli>(
                chrono::steady_clock::now() - sent_time).count();

            {
                lock_guard<mutex> lock(mtx);
                if (srtt_ms == 0.0) srtt_ms = rtt_sample;
                else srtt_ms = 0.875 * srtt_ms + 0.125 * rtt_sample;
            }

            // Slide send_base forward (do under lock)
            {
                lock_guard<mutex> lock(mtx);
                while (send_base < total_chunks && packets[send_base].acked)
                    send_base++;
            }
        }
    }

    cout << "DEBUG: ack_listener exiting\n";
}

// sender_loop: decide which packets to send without holding mtx during send_packet
void sender_loop()
{
    cout << "DEBUG: sender_loop started (send_base=" << send_base.load()
         << " total_chunks=" << total_chunks << ")\n";

    while (send_base < total_chunks)
    {
        // collect packet ids to send (so we don't call send_packet while holding mtx)
        vector<int> to_send;
        {
            lock_guard<mutex> lock(mtx);
            for (int i = 0; i < WINDOW_SIZE; ++i)
            {
                int id = send_base + i;
                if (id >= total_chunks) break;
                if (!packets[id].sent) to_send.push_back(id);
            }
        }

        // send outside the lock
        for (int id : to_send) send_packet(id);

        this_thread::sleep_for(5ms);
    }

    cout << "DEBUG: sender_loop exiting (send_base=" << send_base.load()
         << " total_chunks=" << total_chunks << ")\n";
}

// retransmit_loop: same pattern — assemble list of ids to (re)send, then send outside lock
void retransmit_loop()
{
    cout << "DEBUG: retransmit_loop started (send_base=" << send_base.load()
         << " total_chunks=" << total_chunks << ")\n";

    while (send_base < total_chunks)
    {
        vector<int> to_retx;
        {
            lock_guard<mutex> lock(mtx);
            int end = min(send_base.load() + WINDOW_SIZE, total_chunks);
            for (int id = send_base; id < end; ++id)
            {
                if (packets[id].acked) continue;
                auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                    chrono::steady_clock::now() - packets[id].last_sent).count();
                if (packets[id].send_count == 0 || elapsed > TIMEOUT_MS) {
                    to_retx.push_back(id);
                }
            }
        }

        for (int id : to_retx) send_packet(id);

        this_thread::sleep_for(10ms);
    }

    cout << "DEBUG: retransmit_loop exiting (send_base=" << send_base.load()
         << " total_chunks=" << total_chunks << ")\n";
}

// metrics printer every 1s
void metrics_loop()
{
    cout << "DEBUG: metrics_loop started\n";
    while (total_acks < total_chunks)
    {
        this_thread::sleep_for(1s);

        int acks = total_acks.load();
        long long sent_b = sent_bytes_total.load();
        long long retx = retrans_total.load();

        double srtt_copy = 0.0;
        int inflight = 0;

        {
            lock_guard<mutex> lock(mtx);
            srtt_copy = srtt_ms;
            int start = send_base.load();
            int end = min(start + WINDOW_SIZE, total_chunks);
            for (int i = start; i < end; ++i)
                if (packets[i].sent && !packets[i].acked) inflight++;
        }

        double util = (double)inflight / (double)WINDOW_SIZE * 100.0;

        cout << fixed << setprecision(1);
        cout << "[metrics] acks=" << acks
             << " sent_bytes=" << sent_b
             << " retrans=" << retx
             << " window_util=" << util << "%";

        if (srtt_copy > 0.0)
            cout << " srtt_ms=" << srtt_copy;

        cout << "\n";
    }
    cout << "DEBUG: metrics_loop exiting\n";
}

int main(int argc, char** argv)
{
    debug_print_header();

    if (argc < 2) {
        cout << "Usage: udp_sender_sr.exe <file>\n";
        return 1;
    }

    string filename = argv[1];

    ifstream f(filename, ios::binary);
    if (!f) {
        cout << "Cannot open file: " << filename << "\n";
        return 1;
    }

    f.seekg(0, ios::end);
    uint64_t file_size = (uint64_t)f.tellg();
    f.seekg(0, ios::beg);

    cout << "DEBUG: file_size=" << file_size << "\n";

    total_chunks = static_cast<int>((file_size + CHUNK_SIZE - 1) / CHUNK_SIZE);
    cout << "DEBUG: total_chunks computed=" << total_chunks << "\n";

    packets.resize(max(1, total_chunks));

    for (int i = 0; i < total_chunks; ++i)
    {
        uint64_t offset = (uint64_t)i * CHUNK_SIZE;
        uint64_t remaining = (file_size > offset) ? (file_size - offset) : 0;
        int sz = static_cast<int>(min<uint64_t>((uint64_t)CHUNK_SIZE, remaining));
        packets[i].data.resize(sz);
        if (sz > 0) {
            f.read(packets[i].data.data(), sz);
            cout << "DEBUG: loaded chunk " << i << " size=" << sz << "\n";
        } else {
            cout << "DEBUG: chunk " << i << " size=0\n";
        }
    }

    // Init Winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "socket() failed\n";
        return 1;
    }

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &recv_addr.sin_addr);

    // send META
    PacketHeader meta{};
    meta.type = 3;
    meta.file_size = file_size;
    meta.id = static_cast<uint32_t>(total_chunks);

    int meta_len = sendto(sock, (char*)&meta, sizeof(meta), 0,
                   (sockaddr*)&recv_addr, sizeof(recv_addr));
    if (meta_len > 0) sent_bytes_total += meta_len;

    cout << "[sender] META sent: file=" << filename
         << " size=" << file_size
         << " chunks=" << total_chunks << "\n";

    // start threads
    thread t_ack(ack_listener);
    thread t_sender(sender_loop);
    thread t_retx(retransmit_loop);
    thread t_metrics(metrics_loop);

    auto tstart = chrono::steady_clock::now();

    t_ack.join();
    t_sender.join();
    t_retx.join();
    t_metrics.join();

    auto tend = chrono::steady_clock::now();
    double sec = chrono::duration<double>(tend - tstart).count();

    double mb = static_cast<double>(file_size) / (1024.0 * 1024.0);
    double speed = (mb > 0.0 && sec > 0.0) ? (mb / sec) : 0.0;

    cout << fixed << setprecision(4);
    cout << "[sender] Transfer complete: " << mb << " MB in " << sec << " sec => " << speed << " MB/s\n";

    cout << "[summary] acks=" << total_acks << " sent_bytes=" << sent_bytes_total
         << " retrans=" << retrans_total << " srtt_ms=" << srtt_ms << "\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}

