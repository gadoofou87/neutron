#include "stream_manager.hpp"

#include "stream_p.hpp"

namespace protocol {

namespace detail {

std::optional<StreamSequenceNumber::value_type> StreamManager::find_readable() {
  for (const auto& [id, stream] : streams_) {
    if (stream.is_readable()) {
      return id;
    }
  }

  return std::nullopt;
}

Stream& StreamManager::get(StreamSequenceNumber::value_type identifier) {
  return streams_.try_emplace(identifier, parent(), identifier).first->second;
}

StreamPrivate& StreamManager::get_private(StreamSequenceNumber::value_type identifier) {
  return *get(identifier).impl_;
}

void StreamManager::reset() { streams_.clear(); }

}  // namespace detail

}  // namespace protocol
