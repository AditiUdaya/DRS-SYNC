#include "drs_sync/network_interface.hpp"
#include <iostream>

namespace drs_sync {

NetworkInterface::NetworkInterface(asio::io_context& io_context)
    : io_context_(io_context), socket_(io_context) {
}

NetworkInterface::~NetworkInterface() {
    if (socket_.is_open()) {
        asio::error_code ec;
        socket_.close(ec);
    }
}

bool NetworkInterface::bind(const std::string& address, uint16_t port) {
    try {
        asio::ip::udp::endpoint endpoint(
            asio::ip::make_address(address), port);
        socket_.open(endpoint.protocol());
        socket_.bind(endpoint);
        start_receive();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Bind failed: " << e.what() << "\n";
        return false;
    }
}

void NetworkInterface::send_packet(const Packet& packet, 
                                   const asio::ip::udp::endpoint& endpoint) {
    auto buffer = packet.serialize();
    
    asio::error_code ec;
    socket_.send_to(asio::buffer(buffer), endpoint, 0, ec);
    
    if (ec) {
        std::cerr << "Send failed: " << ec.message() << "\n";
    }
}

void NetworkInterface::set_packet_callback(PacketCallback callback) {
    packet_callback_ = std::move(callback);
}

asio::ip::udp::endpoint NetworkInterface::local_endpoint() const {
    return socket_.local_endpoint();
}

void NetworkInterface::start_receive() {
    socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](const asio::error_code& error, size_t bytes_received) {
            handle_receive(error, bytes_received);
        });
}

void NetworkInterface::handle_receive(const asio::error_code& error, 
                                      size_t bytes_received) {
    if (!error && bytes_received > 0) {
        Packet packet = Packet::deserialize(recv_buffer_.data(), bytes_received);
        
        if (packet_callback_) {
            packet_callback_(packet, remote_endpoint_);
        }
    }
    
    // Continue receiving
    start_receive();
}

} // namespace drs_sync