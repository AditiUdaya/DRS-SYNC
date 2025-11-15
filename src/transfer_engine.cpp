#include "drs_sync/transfer_engine.hpp"
#include "drs_sync/integrity.hpp"
#include <fstream>
#include <iostream>
#include <random>

namespace drs_sync {

TransferEngine::TransferEngine(asio::io_context& io_context)
    : io_context_(io_context),
      network_(std::make_unique<NetworkInterface>(io_context)),
      checkpoint_(std::make_unique<SimpleCheckpoint>()),
      running_(true) {
    
    // Bind to default port
    if (!network_->bind("0.0.0.0", 9090)) {
        std::cerr << "Failed to bind network interface\n";
        return;
    }
    
    // Set packet callback
    network_->set_packet_callback(
        [this](const Packet& pkt, const asio::ip::udp::endpoint& ep) {
            handle_incoming_packet(pkt, ep);
        });
    
    // Start worker threads
    worker_threads_.emplace_back([this]() { sender_loop(); });
    worker_threads_.emplace_back([this]() { receiver_loop(); });
    worker_threads_.emplace_back([this]() { retransmit_loop(); });
    worker_threads_.emplace_back([this]() { telemetry_loop(); });
    
    std::cout << "TransferEngine started on port 9090\n";
}

TransferEngine::~TransferEngine() {
    stop();
}

std::string TransferEngine::start_transfer(const std::string& filepath,
                                          const asio::ip::udp::endpoint& remote,
                                          Priority priority) {
    // Generate file ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t file_id = dis(gen);
    std::string file_id_str = std::to_string(file_id);
    
    // Get file size
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filepath << "\n";
        return "";
    }
    
    uint64_t file_size = file.tellg();
    file.close();
    
    // Calculate total chunks
    uint32_t total_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    // Create transfer context
    auto ctx = std::make_shared<TransferContext>();
    ctx->file_id = file_id_str;
    ctx->filepath = filepath;
    ctx->file_size = file_size;
    ctx->priority = priority;
    ctx->remote_endpoint = remote;
    ctx->total_chunks = total_chunks;
    ctx->ack_bitmap.resize(total_chunks, false);
    ctx->send_times.resize(total_chunks);
    ctx->congestion = std::make_unique<CongestionControl>();
    ctx->congestion->set_priority(priority);
    ctx->start_time = std::chrono::steady_clock::now();
    
    // Store transfer
    {
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        transfers_[file_id_str] = ctx;
    }
    
    std::cout << "Started transfer: " << file_id_str 
              << " (" << total_chunks << " chunks)\n";
    
    return file_id_str;
}

void TransferEngine::pause_transfer(const std::string& file_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = transfers_.find(file_id);
    if (it != transfers_.end()) {
        it->second->stats.paused = true;
        
        // Save checkpoint
        uint32_t last_acked = 0;
        for (uint32_t i = 0; i < it->second->total_chunks; i++) {
            if (it->second->ack_bitmap[i]) {
                last_acked = i;
            }
        }
        checkpoint_->save_progress(file_id, last_acked, it->second->file_size);
        
        std::cout << "Paused transfer: " << file_id << "\n";
    }
}

void TransferEngine::resume_transfer(const std::string& file_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = transfers_.find(file_id);
    if (it != transfers_.end()) {
        it->second->stats.paused = false;
        std::cout << "Resumed transfer: " << file_id << "\n";
    }
}

TransferStats TransferEngine::get_stats(const std::string& file_id) {
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = transfers_.find(file_id);
    if (it != transfers_.end()) {
        return it->second->stats;
    }
    return TransferStats{};
}

void TransferEngine::stop() {
    running_ = false;
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "TransferEngine stopped\n";
}

void TransferEngine::sender_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        
        for (auto& [file_id, ctx] : transfers_) {
            if (ctx->stats.paused || ctx->stats.completed) {
                continue;
            }
            
            std::lock_guard<std::mutex> ctx_lock(ctx->mutex);
            
            uint32_t window_size = ctx->congestion->get_window_size();
            
            // Find chunks to send
            for (uint32_t chunk_id = 0; chunk_id < ctx->total_chunks; chunk_id++) {
                if (!ctx->ack_bitmap[chunk_id] && is_chunk_in_window(*ctx, chunk_id)) {
                    send_chunk(*ctx, chunk_id);
                }
            }
        }
    }
}

void TransferEngine::receiver_loop() {
    // This is handled by the async callback in NetworkInterface
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TransferEngine::retransmit_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        
        for (auto& [file_id, ctx] : transfers_) {
            if (ctx->stats.paused || ctx->stats.completed) {
                continue;
            }
            
            std::lock_guard<std::mutex> ctx_lock(ctx->mutex);
            
            auto now = std::chrono::steady_clock::now();
            auto timeout = ctx->congestion->get_retry_timeout();
            
            // Check for timeouts
            for (uint32_t chunk_id = 0; chunk_id < ctx->total_chunks; chunk_id++) {
                if (!ctx->ack_bitmap[chunk_id]) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - ctx->send_times[chunk_id]);
                    
                    if (elapsed > timeout) {
                        // Retransmit
                        ctx->congestion->on_packet_loss();
                        send_chunk(*ctx, chunk_id);
                        ctx->stats.retransmissions++;
                    }
                }
            }
        }
    }
}

void TransferEngine::telemetry_loop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(transfers_mutex_);
        
        for (auto& [file_id, ctx] : transfers_) {
            if (ctx->stats.completed) continue;
            
            std::lock_guard<std::mutex> ctx_lock(ctx->mutex);
            
            // Update throughput
            double throughput = ctx->congestion->get_throughput_mbps();
            ctx->stats.throughput_mbps = throughput;
            
            // Check if completed
            bool all_acked = true;
            for (bool acked : ctx->ack_bitmap) {
                if (!acked) {
                    all_acked = false;
                    break;
                }
            }
            
            if (all_acked) {
                ctx->stats.completed = true;
                checkpoint_->clear(file_id);
                
                auto elapsed = std::chrono::steady_clock::now() - ctx->start_time;
                double seconds = std::chrono::duration<double>(elapsed).count();
                
                std::cout << "Transfer completed: " << file_id 
                          << " in " << seconds << "s\n";
            }
        }
    }
}

void TransferEngine::handle_incoming_packet(const Packet& packet, 
                                           const asio::ip::udp::endpoint& endpoint) {
    if (packet.header.type != PacketType::ACK) {
        return;
    }
    
    std::string file_id = std::to_string(packet.header.file_id);
    
    std::lock_guard<std::mutex> lock(transfers_mutex_);
    auto it = transfers_.find(file_id);
    if (it == transfers_.end()) {
        return;
    }
    
    auto& ctx = it->second;
    std::lock_guard<std::mutex> ctx_lock(ctx->mutex);
    
    uint32_t chunk_id = packet.header.seq_id;
    
    if (chunk_id < ctx->total_chunks && !ctx->ack_bitmap[chunk_id]) {
        ctx->ack_bitmap[chunk_id] = true;
        ctx->stats.chunks_acked++;
        ctx->stats.bytes_acked += std::min<uint64_t>(
            CHUNK_SIZE, 
            ctx->file_size - chunk_id * CHUNK_SIZE
        );
        
        // Update congestion control
        auto now = std::chrono::steady_clock::now();
        auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
            now - ctx->send_times[chunk_id]);
        ctx->congestion->update_rtt(rtt);
        ctx->congestion->on_ack_received();
    }
}

void TransferEngine::send_chunk(TransferContext& ctx, uint32_t chunk_id) {
    // Read chunk from file
    std::ifstream file(ctx.filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for reading\n";
        return;
    }
    
    uint64_t offset = static_cast<uint64_t>(chunk_id) * CHUNK_SIZE;
    file.seekg(offset);
    
    size_t chunk_size = std::min<size_t>(
        CHUNK_SIZE, 
        ctx.file_size - offset
    );
    
    std::vector<uint8_t> buffer(chunk_size);
    file.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
    file.close();
    
    // Create packet
    Packet packet;
    packet.header.type = PacketType::DATA;
    packet.header.priority = ctx.priority;
    packet.header.seq_id = chunk_id;
    packet.header.file_id = std::stoull(ctx.file_id);
    packet.header.file_size = ctx.file_size;
    packet.header.data_length = chunk_size;
    packet.header.checksum = Integrity::xxhash32(buffer.data(), chunk_size);
    
    if (chunk_id == ctx.total_chunks - 1) {
        packet.header.flags |= PacketFlags::FINAL_CHUNK;
    }
    
    packet.data = std::move(buffer);
    
    // Send packet
    network_->send_packet(packet, ctx.remote_endpoint);
    
    // Update tracking
    ctx.send_times[chunk_id] = std::chrono::steady_clock::now();
    ctx.stats.chunks_sent++;
    ctx.stats.bytes_sent += chunk_size;
}

bool TransferEngine::is_chunk_in_window(TransferContext& ctx, uint32_t chunk_id) {
    uint32_t window_size = ctx.congestion->get_window_size();
    
    // Find first unacked chunk
    uint32_t base = 0;
    for (uint32_t i = 0; i < ctx.total_chunks; i++) {
        if (!ctx.ack_bitmap[i]) {
            base = i;
            break;
        }
    }
    
    return chunk_id >= base && chunk_id < base + window_size;
}

} // namespace drs_sync