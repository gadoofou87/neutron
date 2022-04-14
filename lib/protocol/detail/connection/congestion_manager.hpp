#pragma once

#include <cstddef>

#include "api/types/transmission_sequence_number.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class CongestionManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  void acknowledged(size_t bytes, bool cum_tsn_ack_point_advanced);

  void enter_fast_recovery(TransmissionSequenceNumber::value_type exit_point);

  void exit_fast_recovery();

  [[nodiscard]] TransmissionSequenceNumber::value_type fast_recover_exit_point() const;

  [[nodiscard]] bool in_fast_recovery() const;

  [[nodiscard]] bool is_transmittable(size_t bytes) const;

  void on_long_idle_period();

  void on_retransmission();

  void reset() override;

  void transmitted(size_t bytes);

 private:
  [[nodiscard]] size_t initial_cwnd() const;

 private:
  size_t bytes_outstanding_;
  size_t cwnd_;
  size_t partial_bytes_acked_;
  size_t ssthresh_;
  bool in_fast_recovery_;
  TransmissionSequenceNumber::value_type fast_recover_exit_point_;
};

}  // namespace detail

}  // namespace protocol
