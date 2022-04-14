#pragma once

#include "serialization/packed_struct.hpp"

namespace protocol {

namespace detail {

class ShutdownAcknowledgement : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  bool validate() override { return true; }
};

}  // namespace detail

}  // namespace protocol
