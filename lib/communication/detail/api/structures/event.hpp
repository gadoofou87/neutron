#pragma once

#include "serialization/buffer_builder.hpp"
#include "serialization/packed_struct.hpp"

namespace communication {

namespace detail {

class Event : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(data_offset(), data_size())) {
      return false;
    }

    return true;
  }

 public:
  using data_type = std::span<uint8_t>;

 private:
  size_t data_offset() { return 0; }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace communication

namespace serialization {

using namespace communication::detail;

template <typename... Tags>
class BufferBuilder<Event, Tags...> {
 public:
  BufferBuilder() : data_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return data_size(); }

  size_t data_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...), "data size is not set");

    return data_size_;
  }

  [[nodiscard]] auto set_data_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...), "data size is already set");

    return BufferBuilder<Event, BufferBuilderTag<0>, Tags...>{
        data_size_ + sizeof(Event::data_type::value_type) * size};
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
