#pragma once

#include <cstdint>

namespace communication {

namespace detail {

enum class MessageType : uint8_t { Event, Exception, RawData, Request, Response };

}

}  // namespace communication
