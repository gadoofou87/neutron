#pragma once

#include <vector>

#include "../types/stream_identifier.hpp"
#include "../types/stream_sequence_number.hpp"
#include "../types/transmission_sequence_number.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class PayloadData : public serialization::PackedStruct {
 public:
  struct Bits {
    bool b : 1;
    bool e : 1;
    bool u : 1;
  };

 public:
  using PackedStruct::PackedStruct;

  auto &bits() { return jmp_ref<bits_type>(bits_offset()); }
  auto &tsn() { return jmp_ref<tsn_type>(tsn_offset()); }
  auto &sid() { return jmp_ref<sid_type>(sid_offset()); }
  auto &ssn() { return jmp_ref<ssn_type>(ssn_offset()); }
  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(bits_offset(), sizeof(bits_type))) {
      return false;
    }
    if (!range_check(tsn_offset(), sizeof(tsn_type))) {
      return false;
    }
    if (!range_check(sid_offset(), sizeof(sid_type))) {
      return false;
    }
    if (!range_check(ssn_offset(), sizeof(ssn_type))) {
      return false;
    }
    if (!range_check(data_offset(), data_size())) {
      return false;
    }

    return true;
  }

 public:
  using bits_type = Bits;
  using tsn_type = serialization::PackedInteger<TransmissionSequenceNumber::value_type>;
  using sid_type = serialization::PackedInteger<StreamIdentifier>;
  using ssn_type = serialization::PackedInteger<StreamSequenceNumber::value_type>;
  using data_type = std::span<uint8_t>;

 private:
  size_t bits_offset() { return 0; }
  size_t tsn_offset() { return bits_offset() + sizeof(bits_type); }
  size_t sid_offset() { return tsn_offset() + sizeof(tsn_type); }
  size_t ssn_offset() { return sid_offset() + sizeof(sid_type); }
  size_t data_offset() { return ssn_offset() + sizeof(ssn_type); }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<PayloadData, Tags...> {
 public:
  static constexpr size_t static_size =
      sizeof(PayloadData::bits_type) + sizeof(PayloadData::tsn_type) +
      sizeof(PayloadData::sid_type) + sizeof(PayloadData::ssn_type);

 public:
  BufferBuilder() : dynamic_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return static_size + dynamic_size(); }

  size_t dynamic_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...), "data size is not set");

    return dynamic_size_;
  }

  [[nodiscard]] auto set_data_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...), "data size is already set");

    return BufferBuilder<PayloadData, BufferBuilderTag<0>, Tags...>{
        dynamic_size_ + sizeof(PayloadData::data_type::value_type) * size};
  }

 private:
  BufferBuilder(size_t dynamic_size) : dynamic_size_(dynamic_size) {}

 private:
  size_t dynamic_size_;

 private:
  template <typename, typename...>
  friend class BufferBuilder;
};

}  // namespace serialization
