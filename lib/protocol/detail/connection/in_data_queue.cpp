#include "in_data_queue.hpp"

#include <cstring>

#include "api/structures/payload_data.hpp"
#include "api/structures/selective_acknowledgement.hpp"
#include "connection_p.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

std::vector<GapAckBlock> InDataQueue::get_gap_ack_blocks() {
  auto iterator = storage_.upper_bound(peer_last_tsn_);

  if (iterator == storage_.end()) {
    return {};
  }

  std::vector<GapAckBlock> result;

  TransmissionSequenceNumber::value_type expected_tsn = peer_last_tsn_ + 1;

  const size_t cached_max_gap_ack_blocks = max_gap_ack_blocks();

  for (; iterator != storage_.end(); ++iterator) {
    PayloadData payload_data(iterator->second.data);

    ASSERT(payload_data.tsn() >= expected_tsn);

    const GapAckOffset offset = payload_data.tsn() - peer_last_tsn_;

    if (payload_data.tsn() == expected_tsn) {
      ASSERT(!result.empty());

      result.back().end = offset;

      ++expected_tsn;
    } else {
      result.emplace_back(GapAckBlock{offset, offset});

      if (result.size() == cached_max_gap_ack_blocks) {
        break;
      }

      expected_tsn = payload_data.tsn() + 1;
    }
  }

  return result;
}

TransmissionSequenceNumber::value_type InDataQueue::peer_last_tsn() const { return peer_last_tsn_; }

InDataQueue::PushReturnValue InDataQueue::push(PayloadData payload_data) {
  storage_type::iterator pos;

  if (auto opt = insert_fragment(payload_data)) {
    pos = *opt;
  } else {
    return {.success = false, .has_packet_loss = false, .user_data = {}};
  }

  shift_peer_last_tsn(pos);

  return {
      .success = true,
      .has_packet_loss = TransmissionSequenceNumber::Greater{}(payload_data.tsn(), peer_last_tsn_),
      .user_data = reassemble_fragments(pos)};
}

void InDataQueue::reset() {
  peer_last_tsn_ = std::numeric_limits<TransmissionSequenceNumber::value_type>::max();
  storage_.clear();
}

InDataQueue::storage_type::value_type& InDataQueue::back() {
  ASSERT(!storage_.empty());

  return *std::prev(storage_.end());
}

std::optional<InDataQueue::storage_type::iterator> InDataQueue::find_beginning_fragment(
    storage_type::iterator iterator) {
  for (TransmissionSequenceNumber::value_type expected_tsn =
           PayloadData(iterator->second.data).tsn();
       ;) {
    PayloadData payload_data(iterator->second.data);

    if (expected_tsn-- != payload_data.tsn()) {
      return std::nullopt;
    }
    if (payload_data.bits().b) {
      return iterator;
    }
    if (iterator-- == storage_.begin()) {
      return std::nullopt;
    }
  }
}

std::optional<InDataQueue::storage_type::iterator> InDataQueue::find_ending_fragment(
    storage_type::iterator iterator) {
  for (TransmissionSequenceNumber::value_type expected_tsn =
           PayloadData(iterator->second.data).tsn();
       ;) {
    PayloadData payload_data(iterator->second.data);

    if (expected_tsn++ != payload_data.tsn()) {
      return std::nullopt;
    }
    if (payload_data.bits().e) {
      return std::next(iterator);
    }
    if (++iterator == storage_.end()) {
      return std::nullopt;
    }
  }
}

std::optional<InDataQueue::storage_type::iterator> InDataQueue::insert_fragment(
    PayloadData payload_data) {
  if (TransmissionSequenceNumber::LessEqual{}(payload_data.tsn(), peer_last_tsn_)) {
    return std::nullopt;
  }

  storage_type::iterator pos;

  if (storage_.empty() || TransmissionSequenceNumber::Less{}(PayloadData(back().second.data).tsn(),
                                                             payload_data.tsn())) {
    pos = storage_.end();
  } else {
    pos = storage_.lower_bound(payload_data.tsn());

    if (pos != storage_.end() && PayloadData(pos->second.data).tsn() == payload_data.tsn()) {
      return std::nullopt;
    }
  }

  storage_type::mapped_type value{.data = {}, .to_be_deleted = false};

  value.data.resize(payload_data.raw_size());
  std::memcpy(value.data.data(), payload_data.raw_data(), value.data.size());

  return storage_.emplace_hint(pos, std::piecewise_construct,
                               std::forward_as_tuple(payload_data.tsn()),
                               std::forward_as_tuple(std::move(value)));
}

size_t InDataQueue::max_gap_ack_blocks() const {
  return (parent().packet_builder.max_chunk_data_size(ChunkType::SelectiveAcknowledgement) -
          serialization::BufferBuilder<SelectiveAcknowledgement>{}
              .set_num_gap_ack_blocks(0)
              .buffer_size()) /
         sizeof(GapAckBlock);
}

std::optional<std::vector<uint8_t>> InDataQueue::reassemble_fragments(storage_type::iterator pos) {
  storage_type::iterator itbeg;
  storage_type::iterator itend;

  if (auto opt = find_beginning_fragment(pos)) {
    itbeg = *opt;
  } else {
    return std::nullopt;
  }
  if (auto opt = find_ending_fragment(pos)) {
    itend = *opt;
  } else {
    return std::nullopt;
  }

  size_t data_size = 0;

  for (auto iterator = itbeg; iterator != itend; ++iterator) {
    data_size += iterator->second.data.size();
  }

  std::vector<uint8_t> user_data(data_size);

  size_t offset = 0;

  for (auto iterator = itbeg; iterator != itend; ++iterator) {
    auto fragment_data = PayloadData(iterator->second.data).data();

    std::memcpy(user_data.data() + offset, fragment_data.data(), fragment_data.size());

    offset += fragment_data.size();

    iterator->second.to_be_deleted = true;
  }

  for (auto iterator = itbeg; iterator != storage_.end(); iterator = storage_.erase(iterator)) {
    if (TransmissionSequenceNumber::Greater{}(PayloadData(iterator->second.data).tsn(),
                                              peer_last_tsn_)) {
      break;
    }
    if (!iterator->second.to_be_deleted) {
      break;
    }
  }

  return user_data;
}

void InDataQueue::shift_peer_last_tsn(storage_type::iterator pos) {
  for (auto iterator = pos; iterator != storage_.end(); ++iterator) {
    PayloadData payload_data(iterator->second.data);

    if (peer_last_tsn_ + 1 == payload_data.tsn()) {
      ++peer_last_tsn_;
    } else {
      break;
    }
  }
}

}  // namespace detail

}  // namespace protocol
