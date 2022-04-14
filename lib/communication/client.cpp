#include "client_p.hpp"
#include "detail/api/structures/request.hpp"
#include "utils/debug/assert.hpp"
#include "utils/span/copy.hpp"

namespace communication {

using namespace detail;

Client::Client()
    : Base(std::static_pointer_cast<BasePrivate>(std::make_shared<ClientPrivate>())),
      impl_(static_cast<ClientPrivate *>(Base::impl_.get())) {}

Client::~Client() = default;

bool Client::has_pending_events() const {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  return !impl_->pending_notifications.empty();
}

std::optional<std::vector<uint8_t>> Client::next_pending_event() {
  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  if (impl_->pending_notifications.empty()) {
    return std::nullopt;
  }

  auto notification = std::move(impl_->pending_notifications.front());
  impl_->pending_notifications.pop_front();

  return notification;
}

void Client::send_request(std::span<const uint8_t> data, const ResponseCallback &on_response,
                          const ExceptionCallback &on_exception) {
  if (on_response == nullptr) {
    throw std::runtime_error("on_response is null");
  }

  std::unique_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  if (impl_->can_send_request()) {
    impl_->send_request(data, on_response, on_exception);
  } else {
    impl_->queue_up_request(data, on_response, on_exception);
  }
}

std::shared_ptr<Client::NewEventEvent> Client::new_event() const { return impl_->new_event_event; }

ClientPrivate::ClientPrivate() : new_event_event(Client::NewEventEvent::create()){};

ClientPrivate::~ClientPrivate() = default;

void ClientPrivate::handle(Event event) {
  if (!event.validate()) {
    return;
  }

  std::vector<uint8_t> data(event.data().size());
  std::memcpy(data.data(), event.data().data(), data.size());

  pending_notifications.push_back(std::move(data));

  new_event_event->emit();
}
bool ClientPrivate::can_send_request() const {
  return unresponded_requests.size() <= std::numeric_limits<RequestIdentifier>::max();
}

void ClientPrivate::handle(Exception exception) {
  if (!exception.validate()) {
    return;
  }

  auto unresponded_requests_iterator = unresponded_requests.find(exception.id());

  if (unresponded_requests_iterator == unresponded_requests.end()) {
    return;
  }

  const auto &callback = std::get<Client::ExceptionCallback>(unresponded_requests_iterator->second);

  if (callback != nullptr) {
    callback(exception.code());
  }

  unresponded_requests.erase(unresponded_requests_iterator);
}

void ClientPrivate::handle(Response response) {
  if (!response.validate()) {
    return;
  }

  auto unresponded_requests_iterator = unresponded_requests.find(response.id());

  if (unresponded_requests_iterator == unresponded_requests.end()) {
    return;
  }

  const auto &callback = std::get<Client::ResponseCallback>(unresponded_requests_iterator->second);

  ASSERT(callback != nullptr);

  std::vector<uint8_t> data(response.data().size());
  std::memcpy(data.data(), response.data().data(), data.size());

  callback(std::move(data));

  unresponded_requests.erase(unresponded_requests_iterator);

  if (!unsent_requests.empty()) {
    auto tuple = std::move(unsent_requests.front());
    unsent_requests.pop_front();

    std::apply([this](auto &&...args) { send_request(std::forward<decltype(args)>(args)...); },
               tuple);
  }
}

void ClientPrivate::queue_up_request(std::span<const uint8_t> data,
                                     const Client::ResponseCallback &on_response,
                                     const Client::ExceptionCallback &on_exception) {
  std::vector<uint8_t> data_copy(data.size());
  std::memcpy(data_copy.data(), data.data(), data_copy.size());

  unsent_requests.emplace_back(std::move(data_copy), on_response, on_exception);
}

void ClientPrivate::send_request(std::span<const uint8_t> data,
                                 const Client::ResponseCallback &on_response,
                                 const Client::ExceptionCallback &on_exception) {
  auto buffer = serialization::BufferBuilder<Request>{}.set_data_size(data.size()).build();

  Request request(buffer);

  request.id() = next_request_id++;

  utils::span::copy<uint8_t>(request.data(), data);

  send_message(MessageType::Request, std::move(buffer));

  unresponded_requests.emplace(request.id(), std::make_tuple(on_response, on_exception));
}

void ClientPrivate::message_handler(Message message) {
  switch (message.type()) {
    case MessageType::Event:
      handle(Event(message.data()));
      break;
    case MessageType::Exception:
      handle(Exception(message.data()));
      break;
    case MessageType::Response:
      handle(Response(message.data()));
      break;
    default:
      return;
  }
}

}  // namespace communication
