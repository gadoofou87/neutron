#pragma once

#include "serialization/packed_integer.hpp"
#include "stream_identifier.hpp"
#include "stream_sequence_number.hpp"

namespace protocol {

namespace detail {

struct ForwardTsnStream {
  serialization::PackedInteger<StreamIdentifier> sid;
  serialization::PackedInteger<StreamSequenceNumber> ssn;
};

}  // namespace detail

}  // namespace protocol
