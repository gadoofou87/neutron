#pragma once

#include "../types/request_identifier.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"

namespace communication {

namespace detail {

class Response : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &id() { return jmp_ref<id_type>(id_offset()); }
  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(id_offset(), sizeof(id_type))) {
      return false;
    }
    if (!range_check(data_offset(), data_size())) {
      return false;
    }

    return true;
  }

 public:
  using id_type = serialization::PackedInteger<RequestIdentifier>;
  using data_type = std::span<uint8_t>;

 private:
  size_t id_offset() { return 0; }
  size_t data_offset() { return id_offset() + sizeof(id_type); }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace communication

namespace serialization {

using namespace communication::detail;

template <typename... Tags>
class BufferBuilder<Response, Tags...> {
 public:
  static constexpr size_t overhead_size = sizeof(Response::id_type);

 public:
  BufferBuilder() : data_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return overhead_size + data_size(); }

  size_t data_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...), "data size is not set");

    return data_size_;
  }

  [[nodiscard]] auto set_data_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...), "data size is already set");

    return BufferBuilder<Response, BufferBuilderTag<0>, Tags...>{
        data_size_ + sizeof(Response::data_type::value_type) * size};
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
