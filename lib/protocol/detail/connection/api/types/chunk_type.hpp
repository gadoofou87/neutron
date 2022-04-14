#pragma once

#include <cstdint>

namespace protocol {

namespace detail {

enum class ChunkType : uint8_t {
  Abort,
  Initiation,
  InitiationAcknowledgement,
  InitiationComplete,
  PayloadData,
  SelectiveAcknowledgement,
  HeartbeatRequest,
  HeartbeatAcknowledgement,
  ShutdownAssociation,
  ShutdownAcknowledgement,
  ShutdownComplete,
  ForwardCumulativeTSN,
};

}

}  // namespace protocol
