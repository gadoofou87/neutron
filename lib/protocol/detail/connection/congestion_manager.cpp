#include "congestion_manager.hpp"

#include "connection_p.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

void CongestionManager::acknowledged(size_t bytes, bool cum_tsn_ack_point_advanced) {
  bytes_outstanding_ -= std::min(bytes_outstanding_, bytes);

  if (!cum_tsn_ack_point_advanced) {
    return;
  }

  if (cwnd_ <= ssthresh_) {
    if (!in_fast_recovery_ && parent().out_data_queue.has_pending()) {
      cwnd_ += std::min<size_t>(bytes, cwnd_);
    }
  } else {
    partial_bytes_acked_ += bytes;

    if (partial_bytes_acked_ >= cwnd_ && parent().out_data_queue.has_pending()) {
      partial_bytes_acked_ -= cwnd_;
      cwnd_ += parent().packet_builder.mtu();
    }
  }
}

void CongestionManager::enter_fast_recovery(TransmissionSequenceNumber::value_type exit_point) {
  ASSERT(!in_fast_recovery_);

  in_fast_recovery_ = true;
  fast_recover_exit_point_ = exit_point;
  ssthresh_ = std::max<size_t>(cwnd_ / 2, 4 * parent().packet_builder.mtu());
  cwnd_ = ssthresh_;
  partial_bytes_acked_ = 0;
}

void CongestionManager::exit_fast_recovery() {
  ASSERT(in_fast_recovery_);

  in_fast_recovery_ = false;
}

TransmissionSequenceNumber::value_type CongestionManager::fast_recover_exit_point() const {
  return fast_recover_exit_point_;
}

bool CongestionManager::in_fast_recovery() const { return in_fast_recovery_; }

bool CongestionManager::is_transmittable(size_t bytes) const {
  return bytes_outstanding_ + bytes <= cwnd_;
}

void CongestionManager::on_long_idle_period() {
  ASSERT(bytes_outstanding_ == 0);

  cwnd_ = initial_cwnd();
  partial_bytes_acked_ = 0;
}

void CongestionManager::on_retransmission() {
  ssthresh_ = std::max<size_t>(cwnd_ / 2, 4 * parent().packet_builder.mtu());
  cwnd_ = 1 * parent().packet_builder.mtu();
  bytes_outstanding_ = 0;
}

void CongestionManager::reset() {
  ssthresh_ = 4 * parent().packet_builder.mtu();
  cwnd_ = initial_cwnd();
  bytes_outstanding_ = 0;
  partial_bytes_acked_ = 0;
  in_fast_recovery_ = false;
}

void CongestionManager::transmitted(size_t bytes) {
  ASSERT(is_transmittable(bytes));

  bytes_outstanding_ += bytes;
}

size_t CongestionManager::initial_cwnd() const {
  return std::min(4 * parent().packet_builder.mtu(),
                  std::max(2 * parent().packet_builder.mtu(), 4380));
}

}  // namespace detail

}  // namespace protocol
