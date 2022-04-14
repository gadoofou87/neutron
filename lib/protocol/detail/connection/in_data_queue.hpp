#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "api/types/gap_ack_block.hpp"
#include "api/types/stream_identifier.hpp"
#include "api/types/stream_sequence_number.hpp"
#include "api/types/transmission_sequence_number.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class PayloadData;

class InDataQueue : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  struct PushReturnValue {
    bool success;
    bool has_packet_loss;
    std::optional<std::vector<uint8_t>> user_data;
  };
  struct StorageValue {
    std::vector<uint8_t> data;
    bool to_be_deleted;
  };

 public:
  using storage_type = std::map<TransmissionSequenceNumber::value_type, StorageValue,
                                TransmissionSequenceNumber::Less>;

 public:
  using Parentable::Parentable;

  void clear();

  std::vector<GapAckBlock> get_gap_ack_blocks();

  [[nodiscard]] TransmissionSequenceNumber::value_type peer_last_tsn() const;

  PushReturnValue push(PayloadData payload_data);

  void reset() override;

 private:
  storage_type::value_type& back();

  std::optional<storage_type::iterator> find_beginning_fragment(storage_type::iterator iterator);

  std::optional<storage_type::iterator> find_ending_fragment(storage_type::iterator iterator);

  [[nodiscard]] size_t max_gap_ack_blocks() const;

  std::optional<storage_type::iterator> insert_fragment(PayloadData payload_data);

  std::optional<std::vector<uint8_t>> reassemble_fragments(storage_type::iterator pos);

  void shift_peer_last_tsn(storage_type::iterator pos);

 private:
  TransmissionSequenceNumber::value_type peer_last_tsn_;
  storage_type storage_;
};

}  // namespace detail

}  // namespace protocol
