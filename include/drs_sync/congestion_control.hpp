#pragma once
#include "packet.hpp"
#include <chrono>
#include <atomic>

namespace drs_sync {

enum class CongestionState {
    SLOW_START,
    CONGESTION_AVOIDANCE,
    FAST_RECOVERY,
    CONGESTED
};

class CongestionControl {
public:
    CongestionControl();
    
    // Update RTT measurement
    void update_rtt(std::chrono::microseconds rtt);
    
    // Report packet loss
    void on_packet_loss();
    
    // Report successful transmission
    void on_ack_received();
    
    // Get current window size
    uint32_t get_window_size() const;
    
    // Get retry timeout
    std::chrono::milliseconds get_retry_timeout() const;
    
    // Get current state
    CongestionState get_state() const { return state_; }
    
    // Set priority (affects window multiplier)
    void set_priority(Priority priority);
    
    // Get statistics
    double get_throughput_mbps() const;
    double get_loss_rate() const;
    std::chrono::microseconds get_avg_rtt() const;
    
private:
    void transition_state(CongestionState new_state);
    double get_priority_multiplier() const;
    
    CongestionState state_;
    Priority priority_;
    
    // Window management
    std::atomic<uint32_t> window_size_;
    uint32_t ssthresh_; // Slow start threshold
    
    static constexpr uint32_t MIN_WINDOW = 8;
    static constexpr uint32_t MAX_WINDOW = 1024;
    static constexpr uint32_t INITIAL_WINDOW = 32;
    
    // RTT tracking (RFC 6298)
    std::atomic<int64_t> srtt_us_; // Smoothed RTT
    std::atomic<int64_t> rttvar_us_; // RTT variance
    
    // Statistics
    std::atomic<uint64_t> packets_sent_;
    std::atomic<uint64_t> packets_lost_;
    std::atomic<uint64_t> bytes_sent_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace drs_sync