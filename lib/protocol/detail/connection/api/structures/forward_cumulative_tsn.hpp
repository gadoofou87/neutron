#pragma once

#include <vector>

#include "../types/forward_tsn_stream.hpp"
#include "../types/transmission_sequence_number.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class ForwardCumulativeTSN : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &new_cumulative_tsn() {
    return jmp_ref<new_cumulative_tsn_type>(new_cumulative_tsn_offset());
  }
  auto streams() {
    return streams_type(jmp_ptr<streams_type::value_type>(streams_offset()), num_streams());
  }

  bool validate() override { return true; }

 public:
  using new_cumulative_tsn_type =
      serialization::PackedInteger<TransmissionSequenceNumber::value_type>;
  using streams_type = std::span<ForwardTsnStream>;

 private:
  size_t new_cumulative_tsn_offset() { return 0; }
  size_t streams_offset() { return new_cumulative_tsn_offset() + sizeof(new_cumulative_tsn_type); }
  size_t num_streams() {
    return (raw_size() - streams_offset()) / sizeof(streams_type::value_type);
  }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<ForwardCumulativeTSN, Tags...> {
 public:
  static constexpr size_t overhead_size = sizeof(ForwardCumulativeTSN::new_cumulative_tsn_type);

 public:
  BufferBuilder() : data_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return overhead_size + data_size(); }

  size_t data_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...), "num streams is not set");

    return data_size_;
  }

  [[nodiscard]] auto set_num_streams(size_t num) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...),
                  "num streams is already set");

    return BufferBuilder<ForwardCumulativeTSN, BufferBuilderTag<0>, Tags...>{
        data_size_ + sizeof(ForwardCumulativeTSN::streams_type::value_type) * num};
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
