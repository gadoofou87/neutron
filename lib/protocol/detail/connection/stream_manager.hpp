#pragma once

#include <unordered_map>

#include "api/types/stream_sequence_number.hpp"
#include "stream.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class StreamManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  std::optional<StreamSequenceNumber::value_type> find_readable();

  Stream& get(StreamSequenceNumber::value_type identifier);

  StreamPrivate& get_private(StreamSequenceNumber::value_type identifier);

  void reset() override;

 private:
  std::unordered_map<StreamSequenceNumber::value_type, Stream> streams_;
};

}  // namespace detail

}  // namespace protocol
