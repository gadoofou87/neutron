#pragma once

#include <cstdint>

namespace communication {

namespace detail {

enum class MessageType : uint8_t {
  Event,
  Exception,
  Request,
  Response,
};

}

}  // namespace communication
