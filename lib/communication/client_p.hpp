#pragma once

#include <unordered_map>

#include "client.hpp"
#include "detail/base_p.hpp"

namespace communication {

namespace detail {

class ClientPrivate : public detail::BasePrivate {
 public:
  explicit ClientPrivate();
  ~ClientPrivate() override;

  bool can_send_request() const;

  template <typename T>
  void handle(T);

  void queue_up_request(size_t stream_identifier, std::span<const uint8_t> data,
                        const Client::ResponseCallback &on_response,
                        const Client::ExceptionCallback &on_exception);

  void send_request(size_t stream_identifier, std::span<const uint8_t> data,
                    const Client::ResponseCallback &on_response,
                    const Client::ExceptionCallback &on_exception);

 public:
  void message_handler(size_t stream_identifier, Message message) override;

 public:
  std::shared_ptr<Client::NewEventEvent> new_event_event;

  size_t next_request_id;
  std::deque<std::vector<uint8_t>> pending_events;
  std::unordered_map<size_t, std::tuple<Client::ResponseCallback, Client::ExceptionCallback>>
      unresponded_requests;
  std::deque<
      std::tuple<size_t, std::vector<uint8_t>, Client::ResponseCallback, Client::ExceptionCallback>>
      unsent_requests;
};

}  // namespace detail

}  // namespace communication
