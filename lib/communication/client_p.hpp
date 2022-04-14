#pragma once

#include <unordered_map>

#include "client.hpp"
#include "detail/api/structures/event.hpp"
#include "detail/api/structures/exception.hpp"
#include "detail/api/structures/response.hpp"
#include "detail/base_p.hpp"

namespace communication {

namespace detail {

class ClientPrivate : public detail::BasePrivate {
 public:
  explicit ClientPrivate();
  ~ClientPrivate() override;

  bool can_send_request() const;

  void handle(Event event);

  void handle(Exception exception);

  void handle(Response response);

  void queue_up_request(std::span<const uint8_t> data, const Client::ResponseCallback &on_response,
                        const Client::ExceptionCallback &on_exception);

  void send_request(std::span<const uint8_t> data, const Client::ResponseCallback &on_response,
                    const Client::ExceptionCallback &on_exception);

 public:
  void message_handler(Message message) override;

 public:
  std::shared_ptr<Client::NewEventEvent> new_event_event;

  size_t next_request_id;
  std::list<std::vector<uint8_t>> pending_notifications;
  std::unordered_map<size_t, std::tuple<Client::ResponseCallback, Client::ExceptionCallback>>
      unresponded_requests;
  std::list<std::tuple<std::vector<uint8_t>, Client::ResponseCallback, Client::ExceptionCallback>>
      unsent_requests;
};

}  // namespace detail

}  // namespace communication
