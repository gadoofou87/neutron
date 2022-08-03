#pragma once

#include "protocol/server.hpp"

namespace detail {

class NetworkManager {
 public:
  explicit NetworkManager(asio::io_context& io_context);

  [[nodiscard]] bool is_accepting() const;

  void start_accept(protocol::Server::Configuration&& configuration);

  void stop_accept();

 private:
  static void new_connection_handler();

 private:
  bool accepting_;
  protocol::Server server_;
  std::shared_ptr<protocol::Server::NewConnectionEvent::Subscription> new_connection_subscription_;
};

}  // namespace detail
