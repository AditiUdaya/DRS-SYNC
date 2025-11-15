#pragma once
#include "packet.hpp"
#include <asio.hpp>
#include <functional>
#include <memory>

namespace drs_sync {

using PacketCallback = std::function<void(const Packet&, const asio::ip::udp::endpoint&)>;

class NetworkInterface {
public:
    NetworkInterface(asio::io_context& io_context);
    ~NetworkInterface();
    
    // Bind to local address and port
    bool bind(const std::string& address, uint16_t port);
    
    // Send packet to endpoint
    void send_packet(const Packet& packet, const asio::ip::udp::endpoint& endpoint);
    
    // Set callback for received packets
    void set_packet_callback(PacketCallback callback);
    
    // Get local endpoint
    asio::ip::udp::endpoint local_endpoint() const;
    
private:
    void start_receive();
    void handle_receive(const asio::error_code& error, size_t bytes_received);
    
    asio::io_context& io_context_;
    asio::ip::udp::socket socket_;
    asio::ip::udp::endpoint remote_endpoint_;
    std::array<uint8_t, Packet::MAX_PACKET_SIZE> recv_buffer_;
    PacketCallback packet_callback_;
};

} // namespace drs_sync