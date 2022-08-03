#include "api/structures/event.hpp"
#include "api/structures/exception.hpp"
#include "api/structures/message.hpp"
#include "api/structures/raw_data.hpp"
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

bool Base::is_open() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->connection != nullptr;
}

bool Base::has_pending_raw_data() const {
  std::unique_lock lock(impl_->mutex);

  return !impl_->pending_raw_data.empty();
}

std::optional<std::vector<uint8_t>> Base::next_pending_raw_data() {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  if (impl_->pending_raw_data.empty()) {
    return std::nullopt;
  }

  auto raw_data = std::move(impl_->pending_raw_data.front());
  impl_->pending_raw_data.pop_front();

  return raw_data;
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

  while (auto stream_identifier = impl_->connection->readable_stream()) {
    auto& stream = (*impl_->connection)[*stream_identifier];

    while (stream.is_readable()) {
      impl_->ready_read_handler(*stream_identifier);
    }
  }
}

void Base::send_raw_data(size_t stream_identifier, std::span<const uint8_t> data) {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  auto buffer = serialization::BufferBuilder<RawData>{}.set_data_size(data.size()).build();

  RawData raw_data(buffer);

  utils::span::copy<uint8_t>(raw_data.data(), data);

  impl_->send_message(stream_identifier, MessageType::RawData, std::move(buffer));
}

std::shared_ptr<Base::NewRawDataEvent> Base::new_raw_data() const {
  return impl_->new_raw_data_event;
}

BasePrivate::BasePrivate() : new_raw_data_event(Base::NewRawDataEvent::create()){};

BasePrivate::~BasePrivate() = default;

void BasePrivate::handle(RawData raw_data) {
  if (!raw_data.validate()) {
    return;
  }

  std::vector<uint8_t> data(raw_data.data().size());
  std::memcpy(data.data(), raw_data.data().data(), data.size());

  pending_raw_data.push_back(std::move(data));

  new_raw_data_event->emit();
}

void BasePrivate::send_message(size_t stream_identifier, MessageType type,
                               std::vector<uint8_t>&& data) {
  switch (type) {
    case MessageType::Event:
      ASSERT(Event(data).validate());
      break;
    case MessageType::Exception:
      ASSERT(Exception(data).validate());
      break;
    case MessageType::RawData:
      ASSERT(RawData(data).validate());
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

  (*connection)[stream_identifier].write(buffer);
}

void BasePrivate::ready_read_handler(size_t stream_identifier) {
  std::unique_lock lock(mutex);

  ASSERT(connection != nullptr);

  while (auto data = (*connection)[stream_identifier].read()) {
    Message message(*data);

    if (!message.validate()) {
      continue;
    }

    BasePrivate::message_handler(stream_identifier, message);
  }
}

void BasePrivate::message_handler(size_t stream_identifier, Message message) {
  switch (message.type()) {
    case MessageType::RawData:
      handle(RawData(message.data()));
      break;
    default:
      message_handler(stream_identifier, message);
      break;
  }
}

}  // namespace communication
