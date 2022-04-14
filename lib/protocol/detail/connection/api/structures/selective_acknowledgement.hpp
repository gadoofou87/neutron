#pragma once

#include <vector>

#include "../types/gap_ack_block.hpp"
#include "../types/transmission_sequence_number.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class SelectiveAcknowledgement : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &cum_tsn_ack() { return jmp_ref<cum_tsn_ack_type>(cum_tsn_ack_offset()); }
  auto gap_ack_blks() {
    return gap_ack_blks_type(jmp_ptr<gap_ack_blks_type::value_type>(gap_ack_blks_offset()),
                             num_gap_ack_blks());
  }

  bool validate() override {
    if (!range_check(cum_tsn_ack_offset(), sizeof(cum_tsn_ack_type))) {
      return false;
    }
    if (!range_check(gap_ack_blks_offset(),
                     sizeof(gap_ack_blks_type::value_type) * num_gap_ack_blks())) {
      return false;
    }

    return true;
  }

 public:
  using cum_tsn_ack_type = serialization::PackedInteger<TransmissionSequenceNumber::value_type>;
  using gap_ack_blks_type = std::span<GapAckBlock>;

 private:
  size_t cum_tsn_ack_offset() { return 0; }
  size_t gap_ack_blks_offset() { return cum_tsn_ack_offset() + sizeof(cum_tsn_ack_type); }
  size_t num_gap_ack_blks() {
    return (raw_size() - gap_ack_blks_offset()) / sizeof(gap_ack_blks_type::value_type);
  }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<SelectiveAcknowledgement, Tags...> {
 public:
  static constexpr size_t overhead_size = sizeof(SelectiveAcknowledgement::cum_tsn_ack_type);

 public:
  BufferBuilder() : data_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return overhead_size + data_size(); }

  size_t data_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...),
                  "num gap ack blocks is not set");

    return data_size_;
  }

  [[nodiscard]] auto set_num_gap_ack_blocks(size_t num) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...),
                  "num gap ack blocks is already set");

    return BufferBuilder<SelectiveAcknowledgement, BufferBuilderTag<0>, Tags...>{
        data_size_ + sizeof(SelectiveAcknowledgement::gap_ack_blks_type::value_type) * num};
  }

 private:
  BufferBuilder(size_t data_size) : data_size_(data_size) {}

 private:
  size_t data_size_;

 private:
  template <typename, typename...>
  friend class BufferBuilder;
};

}  // namespace serialization
