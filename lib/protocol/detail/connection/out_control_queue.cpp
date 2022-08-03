#include "out_control_queue.hpp"

#include "api/structures/abort.hpp"
#include "api/structures/forward_cumulative_tsn.hpp"
#include "api/structures/heartbeat_acknowledgement.hpp"
#include "api/structures/heartbeat_request.hpp"
#include "api/structures/initiation.hpp"
#include "api/structures/initiation_acknowledgement.hpp"
#include "api/structures/initiation_complete.hpp"
#include "api/structures/payload_data.hpp"
#include "api/structures/selective_acknowledgement.hpp"
#include "api/structures/shutdown_acknowledgement.hpp"
#include "api/structures/shutdown_association.hpp"
#include "api/structures/shutdown_complete.hpp"
#include "connection_p.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

void OutControlQueue::clear() { storage_.clear(); }

bool OutControlQueue::empty() const { return storage_.empty(); }

std::list<std::vector<uint8_t>> OutControlQueue::gather_unsent_packets() {
  PacketBuilder::BuildInput input;

  input.reserve(storage_.size());

  for (const auto& sv : storage_) {
    input.emplace_back(sv.type, sv.data);
  }

  auto result = parent().packet_builder.build(std::move(input));

  storage_.clear();

  return result;
}

void OutControlQueue::push(ChunkType type, std::vector<uint8_t> data) {
  switch (type) {
    case ChunkType::Abort:
      ASSERT(Abort(data).validate());
      break;
    case ChunkType::Initiation:
      ASSERT(Initiation(data).validate());
      break;
    case ChunkType::InitiationAcknowledgement:
      ASSERT(InitiationAcknowledgement(data).validate());
      break;
    case ChunkType::InitiationComplete:
      ASSERT(InitiationComplete(data).validate());
      break;
    case ChunkType::PayloadData:
      ASSERT(false);
      break;
    case ChunkType::SelectiveAcknowledgement:
      ASSERT(SelectiveAcknowledgement(data).validate());
      break;
    case ChunkType::HeartbeatRequest:
      ASSERT(HeartbeatRequest(data).validate());
      break;
    case ChunkType::HeartbeatAcknowledgement:
      ASSERT(HeartbeatAcknowledgement(data).validate());
      break;
    case ChunkType::ShutdownAssociation:
      ASSERT(ShutdownAssociation(data).validate());
      break;
    case ChunkType::ShutdownAcknowledgement:
      ASSERT(ShutdownAcknowledgement(data).validate());
      break;
    case ChunkType::ShutdownComplete:
      ASSERT(ShutdownComplete(data).validate());
      break;
    case ChunkType::ForwardCumulativeTSN:
      ASSERT(ShutdownComplete(data).validate());
      break;
  }

  storage_.emplace_back(StorageValue{type, std::move(data)});
}

void OutControlQueue::reset() { storage_.clear(); }

}  // namespace detail

}  // namespace protocol
