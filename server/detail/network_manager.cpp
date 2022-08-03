#include "network_manager.hpp"

#include "server_impl.hpp"

namespace detail {

NetworkManager::NetworkManager(asio::io_context &io_context) : server_(io_context) {}

bool NetworkManager::is_accepting() const { return accepting_; }

void NetworkManager::start_accept(protocol::Server::Configuration &&configuration) {
  ASSERT(accepting_ == false);

  new_connection_subscription_ =
      server_.new_connection()->subscribe([]() { new_connection_handler(); });
  server_.open(std::move(configuration));

  accepting_ = true;
}

void NetworkManager::stop_accept() {
  ASSERT(accepting_ == true);

  server_.close();

  accepting_ = false;
}

void NetworkManager::new_connection_handler() {
  auto &impl = ServerImpl::instance();

  if (!impl.network_manager.is_accepting()) {
    return;
  }

  while (impl.network_manager.server_.has_pending_connections()) {
    auto connection = impl.network_manager.server_.next_pending_connection();

    impl.client_manager.add_unauthorized(std::move(connection));
  }
}

}  // namespace detail
