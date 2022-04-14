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

class Initiation : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

 public:
  auto &public_key_a() { return jmp_ref<public_key_a_type>(public_key_a_offset()); }
  auto &public_key_b() { return jmp_ref<public_key_b_type>(public_key_b_offset()); }
  auto &public_key_b_mac() { return jmp_ref<public_key_b_mac_type>(public_key_b_mac_offset()); }

  bool validate() override {
    if (!range_check(public_key_a_offset() + public_key_a().size_offset(),
                     sizeof(public_key_a_type::size_type))) {
      return false;
    }
    if (public_key_a().size() != 0) {
      if (!range_check(public_key_a_offset() + public_key_a().data_offset(),
                       public_key_a().size())) {
        return false;
      }
    }
    if (!range_check(public_key_b_offset() + public_key_b().size_offset(),
                     sizeof(public_key_b_type::size_type))) {
      return false;
    }
    if (public_key_b().size() != 0) {
      if (!range_check(public_key_b_offset() + public_key_b().data_offset(),
                       public_key_b().size())) {
        return false;
      }
    }
    if (!range_check(public_key_b_mac_offset() + public_key_b_mac().size_offset(),
                     sizeof(public_key_b_mac_type::size_type))) {
      return false;
    }
    if (public_key_b_mac().size() != 0) {
      if (!range_check(public_key_b_mac_offset() + public_key_b_mac().data_offset(),
                       public_key_b_mac().size())) {
        return false;
      }
    }

    return true;
  }

 public:
  using public_key_a_type = serialization::PackedDynamicArray<uint8_t, serialization::pu16>;
  using public_key_b_type = serialization::PackedDynamicArray<uint8_t, serialization::pu16>;
  using public_key_b_mac_type = serialization::PackedDynamicArray<uint8_t, serialization::pu8>;

 private:
  size_t public_key_a_offset() { return 0; }
  size_t public_key_b_offset() {
    return public_key_a_offset() + sizeof(public_key_a_type::size_type) + public_key_a().size();
  }
  size_t public_key_b_mac_offset() {
    return public_key_b_offset() + sizeof(public_key_b_type::size_type) + public_key_b().size();
  }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<Initiation, Tags...> {
 public:
  static constexpr size_t overhead_size = sizeof(Initiation::public_key_a_type::size_type) +
                                          sizeof(Initiation::public_key_b_type::size_type) +
                                          sizeof(Initiation::public_key_b_mac_type::size_type);

 public:
  BufferBuilder() : data_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); };

  size_t buffer_size() { return overhead_size + data_size(); }

  size_t data_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...),
                  "public key a size is not set");
    static_assert((std::is_same_v<Tags, BufferBuilderTag<1>> || ...),
                  "public key b size is not set");
    static_assert((std::is_same_v<Tags, BufferBuilderTag<2>> || ...),
                  "public key b mac size is not set");

    return data_size_;
  }

  [[nodiscard]] auto set_public_key_a_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...),
                  "public key a size is already set");

    ASSERT_X(size <= std::numeric_limits<Initiation::public_key_a_type::size_type>::max(),
             "public key a size is too big");

    return BufferBuilder<Initiation, BufferBuilderTag<0>, Tags...>{
        data_size_ + sizeof(Initiation::public_key_a_type::data_type) * size};
  }

  [[nodiscard]] auto set_public_key_b_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<1>> && ...),
                  "public key b size is already set");

    ASSERT_X(size <= std::numeric_limits<Initiation::public_key_b_type::size_type>::max(),
             "public key b size is too big");

    return BufferBuilder<Initiation, BufferBuilderTag<1>, Tags...>{
        data_size_ + sizeof(Initiation::public_key_b_type::data_type) * size};
  }

  [[nodiscard]] auto set_public_key_b_mac_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<2>> && ...),
                  "public key b mac size is already set");

    ASSERT_X(size <= std::numeric_limits<Initiation::public_key_b_mac_type::size_type>::max(),
             "public key b mac size is too big");

    return BufferBuilder<Initiation, BufferBuilderTag<2>, Tags...>{
        data_size_ + sizeof(Initiation::public_key_b_mac_type::data_type) * size};
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
