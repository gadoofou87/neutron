#pragma once

#include <limits>
#include <vector>

#include "../types/connection_id.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_dynamic_array.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

class InitiationAcknowledgement : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

 public:
  auto &connection_id() { return jmp_ref<connection_id_type>(connection_id_offset()); }
  auto &public_key_a() { return jmp_ref<public_key_a_type>(public_key_a_offset()); }
  auto &public_key_a_mac() { return jmp_ref<public_key_a_mac_type>(public_key_a_mac_offset()); }

  bool validate() override {
    if (!range_check(connection_id_offset(), sizeof(connection_id_type))) {
      return false;
    }
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
    if (!range_check(public_key_a_mac_offset() + public_key_a_mac().size_offset(),
                     sizeof(public_key_a_mac_type::size_type))) {
      return false;
    }
    if (public_key_a_mac().size() != 0) {
      if (!range_check(public_key_a_mac_offset() + public_key_a_mac().data_offset(),
                       public_key_a_mac().size())) {
        return false;
      }
    }
    return true;
  }

 public:
  using connection_id_type = serialization::PackedInteger<ConnectionID>;
  using public_key_a_type = serialization::PackedDynamicArray<uint8_t, serialization::pu16>;
  using public_key_a_mac_type = serialization::PackedDynamicArray<uint8_t, serialization::pu8>;

 private:
  size_t connection_id_offset() { return 0; }
  size_t public_key_a_offset() { return connection_id_offset() + sizeof(connection_id_type); }
  size_t public_key_a_mac_offset() {
    return public_key_a_offset() + sizeof(public_key_a_type::size_type) + public_key_a().size();
  }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<InitiationAcknowledgement, Tags...> {
 public:
  static constexpr size_t static_size =
      sizeof(InitiationAcknowledgement::connection_id_type) +
      sizeof(InitiationAcknowledgement::public_key_a_type::size_type) +
      sizeof(InitiationAcknowledgement::public_key_a_mac_type::size_type);

 public:
  BufferBuilder() : dynamic_size_(0) {}

  auto build() { return std::vector<uint8_t>(buffer_size()); };

  size_t buffer_size() { return static_size + dynamic_size(); }

  size_t dynamic_size() {
    static_assert((std::is_same_v<Tags, BufferBuilderTag<0>> || ...),
                  "public key a size is not set");
    static_assert((std::is_same_v<Tags, BufferBuilderTag<1>> || ...),
                  "public key a mac size is not set");

    return dynamic_size_;
  }

  [[nodiscard]] auto set_public_key_a_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<0>> && ...),
                  "public key a size is already set");

    ASSERT_X(
        size <= std::numeric_limits<InitiationAcknowledgement::public_key_a_type::size_type>::max(),
        "public key a size is too big");

    return BufferBuilder<InitiationAcknowledgement, BufferBuilderTag<0>, Tags...>{
        dynamic_size_ + sizeof(InitiationAcknowledgement::public_key_a_type::data_type) * size};
  }

  [[nodiscard]] auto set_public_key_a_mac_size(size_t size) {
    static_assert((!std::is_same_v<Tags, BufferBuilderTag<1>> && ...),
                  "public key a mac size is already set");

    ASSERT_X(
        size <=
            std::numeric_limits<InitiationAcknowledgement::public_key_a_mac_type::size_type>::max(),
        "public key a mac size is too big");

    return BufferBuilder<InitiationAcknowledgement, BufferBuilderTag<1>, Tags...>{
        dynamic_size_ + sizeof(InitiationAcknowledgement::public_key_a_mac_type::data_type) * size};
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
