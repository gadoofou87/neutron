#include "api/structures/event.hpp"
#include "api/structures/exception.hpp"
#include "api/structures/message.hpp"
#include "api/structures/request.hpp"
#include "api/structures/response.hpp"
#include "base_p.hpp"
#include "protocol/stream.hpp"
#include "utils/debug/assert.hpp"
#include "utils/span/copy.hpp"

namespace communication {

using namespace detail;

Base::Base(std::shared_ptr<BasePrivate> impl) : impl_(std::move(impl)) {}

Base::~Base() = default;

size_t Base::current_read_stream() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->current_read_stream;
}

size_t Base::current_write_stream() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->current_write_stream;
}

bool Base::is_open() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->connection != nullptr;
}

void Base::open(std::shared_ptr<protocol::Connection> connection) {
  std::unique_lock lock(impl_->mutex);

  if (is_open()) {
    throw std::runtime_error("is already open");
  }

  impl_->connection = std::move(connection);
  impl_->ready_read_subscription = impl_->connection->ready_read()->subscribe(
      [weak_impl = impl_->weak_from_this()](auto&&... args) {
        if (auto impl = weak_impl.lock()) {
          impl->ready_read_handler(std::forward<decltype(args)>(args)...);
        }
      });

  auto& stream = (*impl_->connection)[current_read_stream()];

  while (stream.is_readable()) {
    impl_->ready_read_handler(current_read_stream());
  }
}

void Base::set_current_read_stream(size_t stream_identifier) {
  std::unique_lock lock(impl_->mutex);

  if (stream_identifier > impl_->connection->max_num_streams()) {
    throw std::runtime_error("stream_identifier exceeds maximum value");
  }

  impl_->current_read_stream = stream_identifier;
}

void Base::set_current_write_stream(size_t stream_identifier) {
  std::unique_lock lock(impl_->mutex);

  if (stream_identifier > impl_->connection->max_num_streams()) {
    throw std::runtime_error("stream_identifier exceeds maximum value");
  }

  impl_->current_write_stream = stream_identifier;
}

BasePrivate::BasePrivate() : current_read_stream(0), current_write_stream(0){};

BasePrivate::~BasePrivate() = default;

void BasePrivate::send_message(MessageType type, std::vector<uint8_t>&& data) {
  switch (type) {
    case MessageType::Event:
      ASSERT(Event(data).validate());
      break;
    case MessageType::Exception:
      ASSERT(Exception(data).validate());
      break;
    case MessageType::Request:
      ASSERT(Request(data).validate());
      break;
    case MessageType::Response:
      ASSERT(Response(data).validate());
      break;
  }

  auto buffer = serialization::BufferBuilder<Message>{}.set_data_size(data.size()).build();

  Message message(buffer);

  message.type() = type;

  utils::span::copy<uint8_t>(message.data(), data);

  ASSERT(connection != nullptr);

  (*connection)[current_write_stream].write(buffer);
}

void BasePrivate::ready_read_handler(size_t stream_identifier) {
  std::unique_lock lock(mutex);

  if (stream_identifier != current_read_stream) {
    return;
  }

  ASSERT(connection != nullptr);

  while (auto data = (*connection)[stream_identifier].read()) {
    Message message(*data);

    if (!message.validate()) {
      continue;
    }

    message_handler(message);
  }
}

}  // namespace communication
