#pragma once

#include <vector>

#include "../types/nonce.hpp"
#include "crypto/chacha20poly1305.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_static_array.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class EncryptedPacketData : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &mac() { return jmp_ref<mac_type>(mac_offset()); }
  auto &nonce() { return jmp_ref<nonce_type>(nonce_offset()); }
  auto data() { return data_type(jmp_ptr<data_type::value_type>(data_offset()), data_size()); }

  bool validate() override {
    if (!range_check(mac_offset(), sizeof(mac_type))) {
      return false;
    }
    if (!range_check(nonce_offset(), sizeof(nonce_type))) {
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
  using mac_type = serialization::PackedStaticArray<uint8_t, crypto::ChaCha20Poly1305::DigestSize>;
  using nonce_type = serialization::PackedInteger<Nonce>;
  using data_type = std::span<uint8_t>;

 private:
  size_t mac_offset() { return 0; }
  size_t nonce_offset() { return mac_offset() + sizeof(mac_type); }
  size_t data_offset() { return nonce_offset() + sizeof(nonce_type); }
  size_t data_size() { return raw_size() - data_offset(); }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<EncryptedPacketData, Tags...> {
 public:
  static constexpr size_t static_size =
      sizeof(EncryptedPacketData::mac_type) + sizeof(EncryptedPacketData::nonce_type);

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

    return BufferBuilder<EncryptedPacketData, BufferBuilderTag<0>, Tags...>{
        dynamic_size_ + sizeof(EncryptedPacketData::data_type::value_type) * size};
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
