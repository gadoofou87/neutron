#pragma once

#include "../types/gap_ack_offset.hpp"
#include "serialization/packed_integer.hpp"

namespace protocol {

namespace detail {

struct GapAckBlock {
  serialization::PackedInteger<GapAckOffset> start;
  serialization::PackedInteger<GapAckOffset> end;
};

}  // namespace detail

}  // namespace protocol
