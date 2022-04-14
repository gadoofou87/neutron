#pragma once

#include "serialization/packed_integer.hpp"

namespace protocol {

namespace detail {

struct HeartbeatInfo {
  serialization::pi64 time_value;
};

}  // namespace detail

}  // namespace protocol
