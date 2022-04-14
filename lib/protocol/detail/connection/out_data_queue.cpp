#include "out_data_queue.hpp"

#include "connection_p.hpp"
#include "detail/connection/api/structures/forward_cumulative_tsn.hpp"
#include "detail/connection/api/structures/payload_data.hpp"
#include "stream_p.hpp"
#include "utils/debug/assert.hpp"
#include "utils/span/copy.hpp"

namespace protocol {

namespace detail {

size_t OutDataQueue::acknowledge(TransmissionSequenceNumber::value_type cum_tsn_ack_point) {
  ASSERT(TransmissionSequenceNumber::Greater{}(cum_tsn_ack_point, cum_tsn_ack_point_));

  size_t bytes_acked = 0;

  for (auto iterator = storage_sent_.begin(); iterator != storage_sent_.end();
       iterator = storage_sent_.erase(iterator)) {
    PayloadData payload_data(iterator->data);

    if (TransmissionSequenceNumber::Greater{}(payload_data.tsn(), cum_tsn_ack_point)) {
      break;
    }

    cum_tsn_ack_point_ = payload_data.tsn();

    if (iterator->acked) {
      continue;
    }

    bytes_acked += mark_as_acked(*iterator);
  }

  if (TransmissionSequenceNumber::Less{}(advanced_peer_tsn_ack_point_, cum_tsn_ack_point_)) {
    advanced_peer_tsn_ack_point_ = cum_tsn_ack_point_;
  }

  if (parent().congestion_manager.in_fast_recovery()) {
    will_retransmit_fast_ = true;
  }

  return bytes_acked;
}

size_t OutDataQueue::acknowledge(TransmissionSequenceNumber::value_type& htna,
                                 std::span<GapAckBlock> gap_ack_blks) {
  htna = cum_tsn_ack_point_;

  if (gap_ack_blks.empty()) {
    return 0;
  }

  size_t bytes_acked = 0;

  auto iterator = storage_sent_.begin();

  for (const auto& g : gap_ack_blks) {
    if (g.start > g.end) {
      continue;
    }

    const TransmissionSequenceNumber::value_type start = cum_tsn_ack_point_ + g.start;
    const TransmissionSequenceNumber::value_type end = cum_tsn_ack_point_ + g.end;

    auto tsn = start;
    bool ok;

    do {
      ok = false;

      for (; iterator != storage_sent_.end(); ++iterator) {
        PayloadData payload_data(iterator->data);

        if (TransmissionSequenceNumber::Greater{}(payload_data.tsn(), tsn)) {
          break;
        }
        if (payload_data.tsn() == tsn) {
          ok = true;
          break;
        }
      }

      if (!ok) {
        break;
      }
      if (iterator->acked) {
        continue;
      }

      bytes_acked += mark_as_acked(*iterator);

      htna = tsn;
    } while (tsn++ != end);

    if (!ok) {
      break;
    }
  }

  return bytes_acked;
}

TransmissionSequenceNumber::value_type OutDataQueue::advanced_peer_tsn_ack_point() const {
  return advanced_peer_tsn_ack_point_;
}

void OutDataQueue::advance_advanced_peer_tsn_ack_point() {
  for (auto& sv : storage_sent_) {
    if (!sv.abandoned) {
      break;
    }

    advanced_peer_tsn_ack_point_ = PayloadData(sv.data).tsn();
  }

  if (TransmissionSequenceNumber::Greater{}(advanced_peer_tsn_ack_point_, cum_tsn_ack_point_)) {
    const auto forward_tsn_streams = get_forward_tsn_streams();

    auto buffer = serialization::BufferBuilder<ForwardCumulativeTSN>{}
                      .set_num_streams(forward_tsn_streams.size())
                      .build();

    ForwardCumulativeTSN forward_tsn(buffer);

    forward_tsn.new_cumulative_tsn() = advanced_peer_tsn_ack_point_;

    if (!forward_tsn_streams.empty()) {
      utils::span::copy<ForwardTsnStream>(forward_tsn.streams(), forward_tsn_streams);
    }

    parent().out_control_queue.push(ChunkType::ForwardCumulativeTSN, std::move(buffer));
  }
}

TransmissionSequenceNumber::value_type OutDataQueue::cum_tsn_ack_point() const {
  return cum_tsn_ack_point_;
}

bool OutDataQueue::empty() const { return storage_sent_.empty() && storage_send_.empty(); }

std::list<std::vector<uint8_t>> OutDataQueue::gather_fast_retransmission_packets() {
  if (!will_retransmit_fast_) {
    return {};
  }

  will_retransmit_fast_ = false;

  PacketBuilder::BuildInput input;

  size_t remains = parent().packet_builder.max_chunk_data_size(ChunkType::PayloadData);

  for (auto iterator = storage_sent_.begin(); iterator != storage_sent_.end() && remains != 0;
       ++iterator) {
    if (iterator->acked || iterator->transmits > 1 || iterator->miss_indications < 3) {
      continue;
    }

    check_partial_reliability_status(iterator);

    if (iterator->abandoned) {
      continue;
    }

    if (remains < iterator->data.size()) {
      break;
    }

    remains -= iterator->data.size();

    ++iterator->transmits;

    input.emplace_back(ChunkType::PayloadData, iterator->data);
  }

  return parent().packet_builder.build(std::move(input));
}

std::list<std::vector<uint8_t>> OutDataQueue::gather_packets_to_retransmit() {
  PacketBuilder::BuildInput input;

  for (auto iterator = storage_sent_.begin(); iterator != storage_sent_.end(); ++iterator) {
    if (!iterator->retransmit || iterator->acked) {
      continue;
    }

    check_partial_reliability_status(iterator);

    if (iterator->abandoned) {
      continue;
    }

    PayloadData payload_data(iterator->data);

    if (!parent().congestion_manager.is_transmittable(payload_data.data().size())) {
      break;
    }

    iterator->retransmit = false;
    ++iterator->transmits;

    parent().congestion_manager.transmitted(payload_data.data().size());

    input.emplace_back(ChunkType::PayloadData, iterator->data);
  }

  return parent().packet_builder.build(std::move(input));
}

std::list<std::vector<uint8_t>> OutDataQueue::gather_unsent_packets() {
  PacketBuilder::BuildInput input;

  for (auto iterator = storage_send_.begin(); iterator != storage_send_.end();
       iterator = storage_send_.begin()) {
    PayloadData payload_data(iterator->data);

    if (!parent().congestion_manager.is_transmittable(payload_data.data().size())) {
      break;
    }

    payload_data.tsn() = my_next_tsn_++;

    iterator->transmits = 1;
    iterator->time_value = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

    storage_sent_.splice(storage_sent_.cend(), storage_send_, iterator);

    parent().congestion_manager.transmitted(payload_data.data().size());

    input.emplace_back(ChunkType::PayloadData, iterator->data);
  }

  return parent().packet_builder.build(std::move(input));
}

void OutDataQueue::inc_miss_indications(TransmissionSequenceNumber::value_type max_tsn) {
  bool three_missing_reports = false;

  for (auto& sv : storage_sent_) {
    PayloadData payload_data(sv.data);

    if (TransmissionSequenceNumber::GreaterEqual{}(payload_data.tsn(), max_tsn)) {
      break;
    }

    if (sv.acked || sv.abandoned || sv.miss_indications >= 3) {
      continue;
    }
    if (++sv.miss_indications != 3) {
      continue;
    }

    three_missing_reports = true;
  }

  if (!parent().congestion_manager.in_fast_recovery() && three_missing_reports) {
    will_retransmit_fast_ = true;

    parent().congestion_manager.enter_fast_recovery(max_tsn);
  }
}

bool OutDataQueue::has_inflight() const { return !storage_sent_.empty(); }

bool OutDataQueue::has_pending() const { return !storage_send_.empty(); }

void OutDataQueue::mark_all_to_retrasmit() {
  for (auto& sv : storage_sent_) {
    if (sv.acked || sv.abandoned) {
      continue;
    }

    sv.retransmit = true;
  }
}

TransmissionSequenceNumber::value_type OutDataQueue::my_next_tsn() const { return my_next_tsn_; }

void OutDataQueue::push(std::vector<uint8_t> data) {
  PayloadData payload_data(data);

  ASSERT(payload_data.validate());

  storage_send_.emplace_back(StorageValue{false, false, false, 0, 0, -1, std::move(data)});
}

void OutDataQueue::reset() {
  advanced_peer_tsn_ack_point_ = std::numeric_limits<TransmissionSequenceNumber::value_type>::max();
  cum_tsn_ack_point_ = std::numeric_limits<TransmissionSequenceNumber::value_type>::max();
  min_tsn2measure_rtt_ = std::numeric_limits<TransmissionSequenceNumber::value_type>::min();
  my_next_tsn_ = std::numeric_limits<TransmissionSequenceNumber::value_type>::min();
  storage_sent_.clear();
  storage_send_.clear();
  will_retransmit_fast_ = false;
  will_send_forward_tsn_ = false;
}

void OutDataQueue::check_partial_reliability_status(storage_type::iterator iterator) {
  if (iterator->abandoned) {
    return;
  }

  ASSERT(!iterator->acked);

  auto& stream_private = parent().stream_manager.get_private(PayloadData(iterator->data).sid());

  switch (stream_private.reliability_type) {
    case Stream::ReliabilityType::Reliable:
      return;
    case Stream::ReliabilityType::Rexmit:
      if (iterator->transmits < stream_private.reliability_value) {
        return;
      }
      break;
    case Stream::ReliabilityType::Timed:
      if (std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now().time_since_epoch())
                  .count() -
              iterator->time_value <
          stream_private.reliability_value) {
        return;
      }
      break;
  }

  for (; iterator != storage_sent_.end(); ++iterator) {
    iterator->acked = iterator->abandoned = true;

    PayloadData payload_data(iterator->data);

    parent().congestion_manager.acknowledged(payload_data.data().size(), false);

    if (payload_data.bits().e) {
      return;
    }
  }
  for (auto iterator = storage_send_.end(); iterator != storage_send_.end();
       iterator = storage_send_.erase(iterator)) {
    if (PayloadData(iterator->data).bits().e) {
      return;
    }
  }

  ASSERT(false);
}

std::vector<ForwardTsnStream> OutDataQueue::get_forward_tsn_streams() {
  //

  std::vector<ForwardTsnStream> result;

  //

  return result;
}

size_t OutDataQueue::mark_as_acked(StorageValue& sv) {
  sv.acked = true;

  PayloadData payload_data(sv.data);

  if (sv.transmits == 1 &&
      TransmissionSequenceNumber::GreaterEqual{}(payload_data.tsn(), min_tsn2measure_rtt_)) {
    min_tsn2measure_rtt_ = my_next_tsn_;

    const auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count() -
                     sv.time_value;

    ASSERT(rtt >= 0);

    parent().rto_manager.recalculate(rtt);
  }

  return payload_data.data().size();
}

}  // namespace detail

}  // namespace protocol
