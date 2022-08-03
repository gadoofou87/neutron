#include "client.hpp"

#include <api/request.pb.h>

#include "server_impl.hpp"

namespace detail {

Client::Client(uint64_t id, std::shared_ptr<protocol::Connection>&& connection)
    : id_(id), connection_(std::move(connection)), authorized_(false) {
  ASSERT(connection_ != nullptr);

  state_changed_subscription_ = connection_->state_changed()->subscribe(
      [weak_self = weak_from_this()]([[maybe_unused]] auto&&... args) {
        if (auto self = weak_self.lock()) {
          ServerImpl::instance().client_manager.remove(self->id_);
        }
      });

  new_raw_data_subscription_ =
      communication_.new_raw_data()->subscribe([weak_self = weak_from_this()](auto&&... args) {
        if (auto self = weak_self.lock()) {
          self->new_raw_data_event_communication_handler(std::forward<decltype(args)>(args)...);
        }
      });
  new_request_subscription_ =
      communication_.new_request()->subscribe([weak_self = weak_from_this()](auto&&... args) {
        if (auto self = weak_self.lock()) {
          self->new_request_event_communication_handler(std::forward<decltype(args)>(args)...);
        }
      });

  communication_.open(connection_);
}

Client::~Client() = default;

const communication::Server& Client::communication() const { return communication_; }

communication::Server& Client::communication() { return communication_; }

uint32_t Client::device_id() const {
  ASSERT(authorized_);

  return device_id_;
}

uint64_t Client::id() const { return id_; }

bool Client::is_authorized() const { return authorized_; }

uint64_t Client::user_id() const {
  ASSERT(authorized_);

  return user_id_;
}

void Client::new_raw_data_event_communication_handler() {
  // stub
}

void Client::new_request_event_communication_handler() {
  while (auto pair_opt = communication_.next_pending_request()) {
    auto& [id, data] = *pair_opt;

    api::Request request;
    if (request.ParseFromArray(data.data(), data.size())) {
      ServerImpl::instance().request_handler.handle(*this, id, request);
    }
  }
}

}  // namespace detail
