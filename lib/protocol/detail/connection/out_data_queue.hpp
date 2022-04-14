#pragma once

#include <cstdint>
#include <list>
#include <span>
#include <vector>

#include "api/types/forward_tsn_stream.hpp"
#include "api/types/gap_ack_block.hpp"
#include "api/types/transmission_sequence_number.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class PayloadData;

struct OutDataQueue : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  struct StorageValue {
    bool abandoned;
    bool acked;
    bool retransmit;
    uint8_t miss_indications;
    uint8_t transmits;
    int64_t time_value;
    std::vector<uint8_t> data;
  };

 public:
  using storage_type = std::list<StorageValue>;

 public:
  using Parentable::Parentable;

  size_t acknowledge(TransmissionSequenceNumber::value_type cum_tsn_ack_point);

  size_t acknowledge(TransmissionSequenceNumber::value_type& htna,
                     std::span<GapAckBlock> gap_ack_blks);

  [[nodiscard]] TransmissionSequenceNumber::value_type advanced_peer_tsn_ack_point() const;

  void advance_advanced_peer_tsn_ack_point();

  void clear();

  [[nodiscard]] TransmissionSequenceNumber::value_type cum_tsn_ack_point() const;

  [[nodiscard]] bool empty() const;

  std::list<std::vector<uint8_t>> gather_fast_retransmission_packets();

  std::list<std::vector<uint8_t>> gather_packets_to_retransmit();

  std::list<std::vector<uint8_t>> gather_unsent_packets();

  [[nodiscard]] bool has_inflight() const;

  [[nodiscard]] bool has_pending() const;

  void inc_miss_indications(TransmissionSequenceNumber::value_type max_tsn);

  void mark_all_to_retrasmit();

  [[nodiscard]] TransmissionSequenceNumber::value_type my_next_tsn() const;

  void push(std::vector<uint8_t> data);

  void reset() override;

 private:
  void check_partial_reliability_status(storage_type::iterator iterator);

  std::vector<ForwardTsnStream> get_forward_tsn_streams();

  size_t mark_as_acked(StorageValue& sv);

 private:
  TransmissionSequenceNumber::value_type advanced_peer_tsn_ack_point_;
  TransmissionSequenceNumber::value_type cum_tsn_ack_point_;
  TransmissionSequenceNumber::value_type min_tsn2measure_rtt_;
  TransmissionSequenceNumber::value_type my_next_tsn_;
  storage_type storage_send_;
  storage_type storage_sent_;
  bool will_retransmit_fast_;
  bool will_send_forward_tsn_;
};

}  // namespace detail

}  // namespace protocol
