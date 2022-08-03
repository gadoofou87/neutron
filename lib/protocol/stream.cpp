#include "connection_p.hpp"
#include "detail/connection/api/structures/payload_data.hpp"
#include "stream_p.hpp"
#include "utils/span/copy.hpp"

namespace protocol {

using namespace detail;

template <>
Stream::Stream(ConnectionPrivate &connection_private, StreamIdentifier &stream_identifier)
    : impl_(new StreamPrivate(connection_private, stream_identifier)) {}

Stream::~Stream() = default;

bool Stream::is_readable() const {
  std::unique_lock lock(impl_->connection_private.mutex);

  return impl_->is_readable_unordered() || impl_->is_readable_ordered();
}

std::optional<std::vector<uint8_t>> Stream::read() {
  std::unique_lock lock(impl_->connection_private.mutex);

  if (impl_->is_readable_unordered()) {
    auto result = std::move(impl_->unordered_queue.front());
    impl_->unordered_queue.pop_front();
    return result;
  }
  if (impl_->is_readable_ordered()) {
    impl_->next_ssn += 1;
    auto result = std::move(impl_->ordered_queue.begin()->second);
    impl_->ordered_queue.erase(impl_->ordered_queue.begin());
    return result;
  }

  return std::nullopt;
}

void Stream::set_reliability_params(bool unordered, ReliabilityType rel_type,
                                    ReliabilityValue rel_val) {
  std::unique_lock lock(impl_->connection_private.mutex);

  impl_->unordered = unordered;
  impl_->reliability_type = rel_type;
  impl_->reliability_value = rel_val;
}

void Stream::write(std::span<const uint8_t> message) {
  if (message.empty()) {
    return;
  }

  std::unique_lock lock(impl_->connection_private.mutex);

  if (impl_->connection_private.state_manager.any_of(
          Connection::State::ShutdownPending, Connection::State::ShutdownSent,
          Connection::State::ShutdownReceived, Connection::State::ShutdownAckSent)) {
    return;
  }

  const size_t cached_max_payload_size = impl_->max_payload_size();

  size_t offset = 0;

  while (offset != message.size()) {
    auto fragment =
        message.subspan(offset, std::min(cached_max_payload_size, message.size() - offset));

    auto buffer =
        serialization::BufferBuilder<PayloadData>{}.set_data_size(fragment.size()).build();

    PayloadData payload_data(buffer);

    payload_data.bits().b = (offset == 0);
    offset += fragment.size();
    payload_data.bits().e = (offset == message.size());
    payload_data.bits().u = impl_->unordered;

    payload_data.sid() = impl_->stream_identifier;
    payload_data.ssn() = impl_->sequence_number;

    utils::span::copy<uint8_t>(payload_data.data(), fragment);

    impl_->connection_private.out_data_queue.push(std::move(buffer));
  }

  ++impl_->sequence_number;

  impl_->connection_private.network_manager.write_pending_packets();
}

StreamPrivate::StreamPrivate(ConnectionPrivate &connection_private,
                             StreamIdentifier stream_identifier)
    : connection_private(connection_private),
      next_ssn(std::numeric_limits<StreamSequenceNumber::value_type>::min()),
      reliability_type(Stream::ReliabilityType::Reliable),
      sequence_number(std::numeric_limits<StreamSequenceNumber::value_type>::min()),
      stream_identifier(stream_identifier),
      unordered(false) {}

StreamPrivate::~StreamPrivate() = default;

void StreamPrivate::handle_data(bool unordered, StreamSequenceNumber::value_type ssn,
                                std::vector<uint8_t> &&message) {
  bool readable;

  if (unordered) {
    unordered_queue.emplace_back(std::move(message));

    readable = is_readable_unordered();
  } else {
    if (!ordered_queue.try_emplace(ssn, std::move(message)).second) {
      return;
    }

    readable = is_readable_ordered();
  }

  if (readable) {
    connection_private.ready_read_event->emit(stream_identifier);
  }
}

bool StreamPrivate::is_readable_ordered() const {
  return !ordered_queue.empty() && ordered_queue.begin()->first == next_ssn;
}

bool StreamPrivate::is_readable_unordered() const { return !unordered_queue.empty(); }

size_t StreamPrivate::max_payload_size() const {
  return (connection_private.packet_builder.max_chunk_data_size(ChunkType::PayloadData) -
          serialization::BufferBuilder<PayloadData>{}.set_data_size(0).buffer_size());
}

}  // namespace protocol
