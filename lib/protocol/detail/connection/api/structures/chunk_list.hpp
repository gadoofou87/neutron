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
  BufferBuilder() : data_size_(0), overhead_size_(sizeof(ChunkList::size_type)) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return overhead_size() + data_size(); }

  size_t data_size() { return data_size_; }

  size_t overhead_size() { return overhead_size_; }

  [[nodiscard]] auto add_chunk_data_size(size_t size) {
    ASSERT_X(size <= std::numeric_limits<ChunkList::chunk_data_type::size_type>::max(),
             "chunk data size is too big");

    return BufferBuilder<ChunkList>{
        data_size_ + sizeof(ChunkList::chunk_data_type::data_type) * size,
        overhead_size_ + sizeof(ChunkList::chunk_data_type::size_type)};
  }

 private:
  BufferBuilder(size_t data_size, size_t overhead_size)
      : data_size_(data_size), overhead_size_(overhead_size) {}

 private:
  size_t data_size_;
  size_t overhead_size_;

 private:
  template <typename, typename...>
  friend class BufferBuilder;
};

}  // namespace serialization
