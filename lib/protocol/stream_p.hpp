#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <vector>

#include "detail/connection/api/types/stream_identifier.hpp"
#include "detail/connection/api/types/stream_sequence_number.hpp"
#include "stream.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class StreamPrivate {
 public:
  explicit StreamPrivate(ConnectionPrivate &connection_private, StreamIdentifier stream_identifier);
  virtual ~StreamPrivate();

 public:
  void handle_data(bool unordered, StreamSequenceNumber::value_type ssn,
                   std::vector<uint8_t> &&user_data);

  [[nodiscard]] bool is_readable_ordered() const;

  [[nodiscard]] bool is_readable_unordered() const;

  size_t max_payload_size() const;

 public:
  ConnectionPrivate &connection_private;

  StreamSequenceNumber::value_type next_ssn;
  std::map<StreamSequenceNumber::value_type, std::vector<uint8_t>, StreamSequenceNumber::Less>
      ordered_queue;
  Stream::ReliabilityType reliability_type;
  Stream::ReliabilityValue reliability_value;
  StreamSequenceNumber::value_type sequence_number;
  StreamIdentifier stream_identifier;
  bool unordered;
  std::list<std::vector<uint8_t>> unordered_queue;
};

}  // namespace detail

}  // namespace protocol
