#include "drs_sync/congestion_control.hpp"
#include <algorithm>

namespace drs_sync {

CongestionControl::CongestionControl()
    : state_(CongestionState::SLOW_START),
      priority_(Priority::NORMAL),
      window_size_(INITIAL_WINDOW),
      ssthresh_(MAX_WINDOW / 2),
      srtt_us_(0),
      rttvar_us_(0),
      packets_sent_(0),
      packets_lost_(0),
      bytes_sent_(0),
      start_time_(std::chrono::steady_clock::now()) {
}

void CongestionControl::update_rtt(std::chrono::microseconds rtt) {
    int64_t rtt_us = rtt.count();
    
    if (srtt_us_ == 0) {
        // First measurement
        srtt_us_ = rtt_us;
        rttvar_us_ = rtt_us / 2;
    } else {
        // RFC 6298 algorithm
        int64_t current_srtt = srtt_us_.load();
        int64_t current_rttvar = rttvar_us_.load();
        
        int64_t diff = rtt_us - current_srtt;
        int64_t new_rttvar = (3 * current_rttvar + std::abs(diff)) / 4;
        int64_t new_srtt = (7 * current_srtt + rtt_us) / 8;
        
        rttvar_us_ = new_rttvar;
        srtt_us_ = new_srtt;
    }
}

void CongestionControl::on_packet_loss() {
    packets_lost_++;
    
    uint32_t current = window_size_.load();
    
    switch (state_) {
        case CongestionState::SLOW_START:
        case CongestionState::CONGESTION_AVOIDANCE:
            ssthresh_ = std::max(current / 2, MIN_WINDOW);
            window_size_ = ssthresh_;
            transition_state(CongestionState::FAST_RECOVERY);
            break;
            
        case CongestionState::FAST_RECOVERY:
            window_size_ = std::max(current * 3 / 4, MIN_WINDOW);
            break;
            
        case CongestionState::CONGESTED:
            window_size_ = std::max(current / 2, MIN_WINDOW);
            break;
    }
}

void CongestionControl::on_ack_received() {
    packets_sent_++;
    
    uint32_t current = window_size_.load();
    double multiplier = get_priority_multiplier();
    
    switch (state_) {
        case CongestionState::SLOW_START:
            // Exponential growth
            window_size_ = std::min(current + 1, 
                                   static_cast<uint32_t>(MAX_WINDOW * multiplier));
            if (current >= ssthresh_) {
                transition_state(CongestionState::CONGESTION_AVOIDANCE);
            }
            break;
            
        case CongestionState::CONGESTION_AVOIDANCE:
            // Linear growth
            if (packets_sent_ % current == 0) {
                window_size_ = std::min(current + 1, 
                                       static_cast<uint32_t>(MAX_WINDOW * multiplier));
            }
            break;
            
        case CongestionState::FAST_RECOVERY:
            // Return to congestion avoidance
            transition_state(CongestionState::CONGESTION_AVOIDANCE);
            break;
            
        case CongestionState::CONGESTED:
            // Slow recovery
            if (packets_sent_ % (current * 2) == 0) {
                window_size_ = std::min(current + 1, 
                                       static_cast<uint32_t>(MAX_WINDOW * multiplier));
                if (get_loss_rate() < 0.01) {
                    transition_state(CongestionState::CONGESTION_AVOIDANCE);
                }
            }
            break;
    }
}

uint32_t CongestionControl::get_window_size() const {
    return window_size_.load();
}

std::chrono::milliseconds CongestionControl::get_retry_timeout() const {
    int64_t srtt = srtt_us_.load();
    int64_t rttvar = rttvar_us_.load();
    
    if (srtt == 0) {
        return std::chrono::milliseconds(1000); // Default 1 second
    }
    
    // RTO = SRTT + 4 * RTTVAR (RFC 6298)
    int64_t rto_us = srtt + 4 * rttvar;
    
    // Apply priority multiplier
    double multiplier = 1.0;
    switch (priority_) {
        case Priority::CRITICAL: multiplier = 0.5; break;
        case Priority::HIGH: multiplier = 0.75; break;
        case Priority::NORMAL: multiplier = 1.0; break;
    }
    
    rto_us = static_cast<int64_t>(rto_us * multiplier);
    
    // Clamp to reasonable range (200ms - 5s)
    rto_us = std::clamp(rto_us, 200000LL, 5000000LL);
    
    return std::chrono::milliseconds(rto_us / 1000);
}

void CongestionControl::set_priority(Priority priority) {
    priority_ = priority;
}

double CongestionControl::get_throughput_mbps() const {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    double seconds = std::chrono::duration<double>(elapsed).count();
    
    if (seconds < 0.001) return 0.0;
    
    double bytes = bytes_sent_.load();
    double mbps = (bytes * 8.0) / (seconds * 1000000.0);
    
    return mbps;
}

double CongestionControl::get_loss_rate() const {
    uint64_t sent = packets_sent_.load();
    uint64_t lost = packets_lost_.load();
    
    if (sent == 0) return 0.0;
    
    return static_cast<double>(lost) / static_cast<double>(sent);
}

std::chrono::microseconds CongestionControl::get_avg_rtt() const {
    return std::chrono::microseconds(srtt_us_.load());
}

void CongestionControl::transition_state(CongestionState new_state) {
    state_ = new_state;
}

double CongestionControl::get_priority_multiplier() const {
    switch (priority_) {
        case Priority::CRITICAL: return 2.0;
        case Priority::HIGH: return 1.5;
        case Priority::NORMAL: return 1.0;
        default: return 1.0;
    }
}

} // namespace drs_sync