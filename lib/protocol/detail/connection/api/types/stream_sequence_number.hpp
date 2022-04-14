#pragma once

#include <cstdint>

#include "../../serial_number.hpp"

namespace protocol {

namespace detail {

using StreamSequenceNumber = SerialNumber<uint16_t>;

}

}  // namespace protocol
