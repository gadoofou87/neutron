#pragma once

#include <list>
#include <unordered_set>

#include "detail/api/structures/request.hpp"
#include "detail/base_p.hpp"
#include "server.hpp"

namespace communication {

namespace detail {

class ServerPrivate : public detail::BasePrivate {
 public:
  explicit ServerPrivate();
  ~ServerPrivate() override;

  void handle(Request request);

 public:
  void message_handler(Message message) override;

 public:
  std::shared_ptr<Server::NewRequestEvent> new_request_event;

  std::list<std::pair<size_t, std::vector<uint8_t>>> pending_requests;
  std::unordered_set<size_t> unresponded_requests;
};

}  // namespace detail

}  // namespace communication
