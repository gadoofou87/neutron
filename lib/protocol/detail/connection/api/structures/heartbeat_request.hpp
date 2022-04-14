#pragma once

#include <vector>

#include "../types/heartbeat_info.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class HeartbeatRequest : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto &hb_info() { return jmp_ref<hb_info_type>(hb_info_offset()); }

  bool validate() override {
    if (!range_check(hb_info_offset(), sizeof(hb_info_type))) {
      return false;
    }

    return true;
  }

 public:
  using hb_info_type = HeartbeatInfo;

 private:
  size_t hb_info_offset() { return 0; }
};

}  // namespace detail

}  // namespace protocol

namespace serialization {

using namespace protocol::detail;

template <typename... Tags>
class BufferBuilder<HeartbeatRequest, Tags...> {
 public:
  BufferBuilder() = default;

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return sizeof(HeartbeatRequest::hb_info_type); }
};

}  // namespace serialization
