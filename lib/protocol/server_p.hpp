#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <asio/io_context_strand.hpp>
#include <asio/steady_timer.hpp>
#include <shared_mutex>
#include <unordered_map>

#include "connection.hpp"
#include "detail/connection/api/types/connection_id.hpp"
#include "server.hpp"

namespace protocol {

namespace detail {

struct ConnectionDetails : public std::recursive_mutex {
  ConnectionDetails(asio::io_context& io_context) : strand(io_context) {}

  enum class State { Connecting, Pending, Accepted, Closing } state;

  asio::io_context::strand strand;

  std::shared_ptr<Connection> connection;
  std::shared_ptr<Connection::StateChangedEvent::Subscription> state_changed_subscription;

  std::shared_ptr<asio::generic::datagram_protocol::socket> rx_socket_server;
  std::shared_ptr<asio::generic::datagram_protocol::socket> tx_socket_server;
  asio::generic::datagram_protocol::endpoint source_endpoint;

  std::list<ConnectionID>::iterator pending_connections_iterator;

  std::optional<asio::steady_timer> closing_timer;
};

class ServerPrivate : public std::enable_shared_from_this<ServerPrivate> {
 public:
  explicit ServerPrivate(asio::io_context& io_context);
  ~ServerPrivate();

 public:
  void async_receive_socket_handler(std::vector<uint8_t> data,
                                    asio::generic::datagram_protocol::endpoint endpoint);

  void async_receive_tx_socket_server_handler(
      ConnectionID connection_id, std::vector<uint8_t> data,
      const asio::generic::datagram_protocol::endpoint& endpoint);

  void async_wait_closing_timer_handler(ConnectionID connection_id);

  void state_changed_event_connection_handler(ConnectionID connection_id,
                                              Connection::State new_state);

 public:
  void start_closing_timer(ConnectionDetails& connection_details, ConnectionID connection_id);

 public:
  asio::io_context& io_context;
  std::shared_mutex mutex;

  size_t backlog;
  std::vector<uint8_t> secret_key;
  std::shared_ptr<asio::generic::datagram_protocol::socket> socket;

  struct : std::unordered_map<ConnectionID, ConnectionDetails>, std::shared_mutex {
    ConnectionID next_connection_id;
  } connections;
  struct : std::list<ConnectionID>, std::recursive_mutex {
  } pending_connections;

  std::shared_ptr<Server::NewConnectionEvent> new_connection_event;
};

}  // namespace detail

}  // namespace protocol
