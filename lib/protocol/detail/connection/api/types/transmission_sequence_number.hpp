#pragma once

#include <cstdint>

#include "../../serial_number.hpp"

namespace protocol {

namespace detail {

using TransmissionSequenceNumber = SerialNumber<uint32_t>;

}

}  // namespace protocol
