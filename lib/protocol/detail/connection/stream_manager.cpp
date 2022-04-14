#include "stream_manager.hpp"

#include "stream_p.hpp"

namespace protocol {

namespace detail {

Stream& StreamManager::get(StreamSequenceNumber::value_type sid) {
  return streams_.try_emplace(sid, parent(), sid).first->second;
}

StreamPrivate& StreamManager::get_private(StreamSequenceNumber::value_type sid) {
  return *get(sid).impl_;
}

void StreamManager::reset() { streams_.clear(); }

}  // namespace detail

}  // namespace protocol
