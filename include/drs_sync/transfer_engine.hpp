#pragma once
#include "packet.hpp"
#include "network_interface.hpp"
#include "congestion_control.hpp"
#include "simple_checkpoint.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace drs_sync {

struct TransferStats {
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_acked{0};
    std::atomic<uint32_t> chunks_sent{0};
    std::atomic<uint32_t> chunks_acked{0};
    std::atomic<uint32_t> retransmissions{0};
    std::atomic<double> throughput_mbps{0.0};
    std::atomic<bool> completed{false};
    std::atomic<bool> paused{false};
    
    // Add copy constructor that loads atomic values
    TransferStats() = default;
    TransferStats(const TransferStats& other) 
        : bytes_sent(other.bytes_sent.load()),
          bytes_acked(other.bytes_acked.load()),
          chunks_sent(other.chunks_sent.load()),
          chunks_acked(other.chunks_acked.load()),
          retransmissions(other.retransmissions.load()),
          throughput_mbps(other.throughput_mbps.load()),
          completed(other.completed.load()),
          paused(other.paused.load()) {}
};

struct TransferContext {
    std::string file_id;
    std::string filepath;
    uint64_t file_size;
    Priority priority;
    asio::ip::udp::endpoint remote_endpoint;
    
    uint32_t total_chunks;
    std::vector<bool> ack_bitmap;
    std::vector<std::chrono::steady_clock::time_point> send_times;
    
    std::unique_ptr<CongestionControl> congestion;
    TransferStats stats;
    
    std::mutex mutex;
    std::chrono::steady_clock::time_point start_time;
};

class TransferEngine {
public:
    TransferEngine(asio::io_context& io_context);
    ~TransferEngine();
    
    // Start a new transfer
    std::string start_transfer(const std::string& filepath,
                              const asio::ip::udp::endpoint& remote,
                              Priority priority = Priority::NORMAL);
    
    // Pause/resume transfer
    void pause_transfer(const std::string& file_id);
    void resume_transfer(const std::string& file_id);
    
    // Get transfer statistics
    TransferStats get_stats(const std::string& file_id);
    
    // Stop engine
    void stop();
    
private:
    void sender_loop();
    void receiver_loop();
    void retransmit_loop();
    void telemetry_loop();
    
    void handle_incoming_packet(const Packet& packet, 
                               const asio::ip::udp::endpoint& endpoint);
    
    void send_chunk(TransferContext& ctx, uint32_t chunk_id);
    bool is_chunk_in_window(TransferContext& ctx, uint32_t chunk_id);
    
    asio::io_context& io_context_;
    std::unique_ptr<NetworkInterface> network_;
    std::unique_ptr<SimpleCheckpoint> checkpoint_;
    
    std::map<std::string, std::shared_ptr<TransferContext>> transfers_;
    std::mutex transfers_mutex_;
    
    std::atomic<bool> running_;
    std::vector<std::thread> worker_threads_;
    
    static constexpr size_t CHUNK_SIZE = 65000;
};

} // namespace drs_sync