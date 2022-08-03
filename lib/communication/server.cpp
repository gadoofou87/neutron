#include "detail/api/structures/event.hpp"
#include "detail/api/structures/exception.hpp"
#include "detail/api/structures/response.hpp"
#include "server_p.hpp"
#include "utils/debug/assert.hpp"
#include "utils/span/copy.hpp"

namespace communication {

using namespace detail;

Server::Server()
    : Base(std::static_pointer_cast<BasePrivate>(std::make_shared<ServerPrivate>())),
      impl_(static_cast<ServerPrivate*>(Base::impl_.get())) {}

Server::~Server() = default;

bool Server::has_pending_requests() const {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  return impl_->pending_requests.empty();
}

size_t Server::max_exception_code() const { return std::numeric_limits<ExceptionCode>::max(); }

std::optional<std::pair<size_t, std::vector<uint8_t>>> Server::next_pending_request() {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  if (impl_->pending_requests.empty()) {
    return std::nullopt;
  }

  auto [request_identifier, stream_identifier, data] = std::move(impl_->pending_requests.front());
  impl_->pending_requests.pop_front();

  impl_->unresponded_requests.emplace(request_identifier, stream_identifier);

  return std::make_pair(request_identifier, std::move(data));
}

void Server::send_event(size_t stream_identifier, std::span<const uint8_t> data) {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  auto buffer = serialization::BufferBuilder<Event>{}.set_data_size(data.size()).build();

  Event event(buffer);

  utils::span::copy<uint8_t>(event.data(), data);

  impl_->send_message(stream_identifier, MessageType::Event, std::move(buffer));
}

void Server::send_exception(size_t request_identifier, size_t code) {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  if (code > max_exception_code()) {
    throw std::runtime_error("code exceeds maximum value");
  }

  auto unresponded_requests_iterator = impl_->unresponded_requests.find(request_identifier);

  if (unresponded_requests_iterator == impl_->unresponded_requests.end()) {
    throw std::runtime_error("request not found");
  }

  auto buffer = serialization::BufferBuilder<Exception>{}.build();

  Exception exception(buffer);

  exception.id() = request_identifier;
  exception.code() = code;

  impl_->send_message(unresponded_requests_iterator->second, MessageType::Exception,
                      std::move(buffer));

  impl_->unresponded_requests.erase(unresponded_requests_iterator);
}

void Server::send_response(size_t request_identifier, std::span<const uint8_t> data) {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  auto unresponded_requests_iterator = impl_->unresponded_requests.find(request_identifier);

  if (unresponded_requests_iterator == impl_->unresponded_requests.end()) {
    throw std::runtime_error("request not found");
  }

  auto buffer = serialization::BufferBuilder<Response>{}.set_data_size(data.size()).build();

  Response response(buffer);

  response.id() = request_identifier;

  utils::span::copy<uint8_t>(response.data(), data);

  impl_->send_message(unresponded_requests_iterator->second, MessageType::Response,
                      std::move(buffer));

  impl_->unresponded_requests.erase(unresponded_requests_iterator);
}

std::shared_ptr<Server::NewRequestEvent> Server::new_request() const {
  return impl_->new_request_event;
}

ServerPrivate::ServerPrivate() : new_request_event(Server::NewRequestEvent::create()) {}

ServerPrivate::~ServerPrivate() = default;

void ServerPrivate::handle(size_t stream_identifier, Request request) {
  if (!request.validate()) {
    return;
  }

  auto pending_requests_iterator =
      std::find_if(pending_requests.begin(), pending_requests.end(),
                   [&](const auto& tuple) { return std::get<0>(tuple) == request.id(); });

  if (pending_requests_iterator != pending_requests.end()) {
    return;
  }

  std::vector<uint8_t> data(request.data().size());
  std::memcpy(data.data(), request.data().data(), data.size());

  pending_requests.emplace_back(request.id(), stream_identifier, std::move(data));

  new_request_event->emit();
}

void ServerPrivate::message_handler(size_t stream_identifier, Message message) {
  switch (message.type()) {
    case MessageType::Request:
      handle(stream_identifier, Request(message.data()));
      break;
    default:
      return;
  }
}

}  // namespace communication
