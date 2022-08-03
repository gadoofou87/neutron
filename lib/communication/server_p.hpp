#pragma once

#include <unordered_map>

#include "detail/api/structures/request.hpp"
#include "detail/base_p.hpp"
#include "server.hpp"

namespace communication {

namespace detail {

class ServerPrivate : public detail::BasePrivate {
 public:
  explicit ServerPrivate();
  ~ServerPrivate() override;

  void handle(size_t stream_identifier, Request request);

 public:
  void message_handler(size_t stream_identifier, Message message) override;

 public:
  std::shared_ptr<Server::NewRequestEvent> new_request_event;

  std::deque<std::tuple<size_t /* request_identifier */, size_t /* stream_identifier */,
                        std::vector<uint8_t>>>
      pending_requests;
  std::unordered_map<size_t /* request_identifier */, size_t /* stream_identifier */>
      unresponded_requests;
};

}  // namespace detail

}  // namespace communication
