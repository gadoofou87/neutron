#pragma once

#include "../types/connection_id.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class Packet : public serialization::PackedStruct {
 public:
  struct Bits {
    bool e : 1;
  };

 public:
  using PackedStruct::PackedStruct;

  auto &bits() { return jmp_ref<bits_type>(bits_offset()); }
  auto &connection_id() { return jmp_ref<connection_id_type>(connection_id_offset()); }
  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(bits_offset(), sizeof(bits_type))) {
      return false;
    }
    if (!range_check(connection_id_offset(), sizeof(connection_id_type))) {
      return false;
    }
    if (data_size() != 0) {
      if (!range_check(data_offset(), data_size())) {
        return false;
      }
    }

    return true;
  }

 public:
  using bits_type = Bits;
  using connection_id_type = serialization::PackedInteger<ConnectionID>;
  using data_type = std::span<uint8_t>;

 private:
  size_t bits_offset() { return 0; }
  size_t connection_id_offset() { return bits_offset() + sizeof(bits_type); }
  size_t data_offset() { return connection_id_offset() + sizeof(connection_id_type); }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<Packet, Tags...> {
 public:
  static constexpr size_t static_size =
      sizeof(Packet::bits_type) + sizeof(Packet::connection_id_type);

 public:
  BufferBuilder() : dynamic_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); };

  size_t buffer_size() { return static_size + dynamic_size(); }

  size_t dynamic_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...), "data size is not set");

    return dynamic_size_;
  }

  [[nodiscard]] auto set_data_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...), "data size is already set");

    return BufferBuilder<Packet, BufferBuilderTag<0>, Tags...>{
        dynamic_size_ + sizeof(Packet::data_type::value_type) * size};
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
