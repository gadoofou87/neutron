#pragma once

#include <vector>

#include "../types/chunk_type.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_enum.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class Chunk : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &type() { return jmp_ref<type_type>(type_offset()); }
  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(type_offset(), sizeof(type_type))) {
      return false;
    }
    if (!range_check(data_offset(), data_size())) {
      return false;
    }

    return true;
  }

 public:
  using type_type = serialization::PackedEnum<ChunkType>;
  using data_type = std::span<uint8_t>;

 private:
  size_t type_offset() { return 0; }
  size_t data_offset() { return type_offset() + sizeof(type_type); }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<Chunk, Tags...> {
 public:
  static constexpr size_t static_size = sizeof(Chunk::type_type);

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

    return BufferBuilder<Chunk, BufferBuilderTag<0>, Tags...>{
        dynamic_size_ + sizeof(Chunk::data_type::value_type) * size};
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
