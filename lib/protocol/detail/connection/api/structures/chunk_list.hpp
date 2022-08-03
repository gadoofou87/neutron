#pragma once

#include <limits>
#include <vector>

#include "serialization/buffer_builder.hpp"
#include "serialization/packed_dynamic_array.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

class ChunkList : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &size() { return jmp_ref<size_type>(size_offset()); }
  auto &chunk_data(size_t n) { return jmp_ref<chunk_data_type>(chunk_data_offset(n)); }

  bool validate() override {
    if (!range_check(size_offset(), sizeof(size_type))) {
      return false;
    }
    for (size_t i = 0; i < size(); ++i) {
      if (!range_check(chunk_data_offset(i) + chunk_data(i).size_offset(),
                       sizeof(chunk_data_type::size_type))) {
        return false;
      }
      if (!range_check(chunk_data_offset(i) + chunk_data(i).data_offset(), chunk_data(i).size())) {
        return false;
      }
    }

    return true;
  }

 public:
  using size_type = serialization::pu8;
  using chunk_data_type = serialization::PackedDynamicArray<uint8_t, serialization::pu16>;

 private:
  size_t size_offset() { return 0; }
  size_t chunk_data_offset(size_t n) {
    if (n == 0) {
      return size_offset() + sizeof(size_type);
    }
    return chunk_data_offset(n - 1) + sizeof(chunk_data_type::size_type) + chunk_data(n - 1).size();
  }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<ChunkList, Tags...> {
 public:
  static constexpr size_t static_size = sizeof(ChunkList::size_type);

 public:
  BufferBuilder() : dynamic_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return static_size + dynamic_size(); }

  size_t dynamic_size() { return dynamic_size_; }

  [[nodiscard]] auto add_chunk_data_size(size_t size) {
    ASSERT_X(size <= std::numeric_limits<ChunkList::chunk_data_type::size_type>::max(),
             "chunk data size is too big");

    return BufferBuilder<ChunkList>{dynamic_size_ + sizeof(ChunkList::chunk_data_type::size_type) +
                                    sizeof(ChunkList::chunk_data_type::data_type) * size};
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
