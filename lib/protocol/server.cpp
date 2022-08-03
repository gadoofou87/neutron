#include <asio/bind_executor.hpp>

#include "crypto/helpers.hpp"
#include "crypto/sidhp434_compressed.hpp"
#include "detail/async_recursive_read_datagram.hpp"
#include "detail/async_send_datagram.hpp"
#include "detail/connection/api/structures/chunk.hpp"
#include "detail/connection/api/structures/chunk_list.hpp"
#include "detail/connection/api/structures/initiation.hpp"
#include "detail/connection/api/structures/packet.hpp"
#include "server_p.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

using namespace detail;

namespace {

constexpr std::chrono::seconds CLOSING_INTERVAL{10};

[[maybe_unused]] void bind_to_loopback_v4(asio::ip::udp::socket& socket) {
  socket.close();
  socket.open(asio::ip::udp::v4());
  socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 0));
}

[[maybe_unused]] void bind_to_loopback_v6(asio::ip::udp::socket& socket) {
  socket.close();
  socket.open(asio::ip::udp::v6());
  socket.bind(asio::ip::udp::endpoint(asio::ip::address_v6::loopback(), 0));
}

[[nodiscard]] std::pair<std::shared_ptr<asio::generic::datagram_protocol::socket>,
                        std::shared_ptr<asio::generic::datagram_protocol::socket>>
create_socket_pair(asio::io_context& io_context) {
  try {
    asio::ip::udp::socket socket1(io_context);
    asio::ip::udp::socket socket2(io_context);

    bool bound = false;

    if (!bound) {
      try {
        bind_to_loopback_v6(socket1);
        bind_to_loopback_v6(socket2);
        bound = true;
      } catch (const asio::system_error&) {
      }
    }

    if (!bound) {
      try {
        bind_to_loopback_v4(socket1);
        bind_to_loopback_v4(socket2);
        bound = true;
      } catch (const asio::system_error&) {
      }
    }

    if (!bound) {
      throw;
    }

    socket1.connect(socket2.local_endpoint());
    socket2.connect(socket1.local_endpoint());

    return std::make_pair(
        std::make_shared<asio::generic::datagram_protocol::socket>(std::move(socket1)),
        std::make_shared<asio::generic::datagram_protocol::socket>(std::move(socket2)));
  } catch (...) {
  }

  return std::make_pair(nullptr, nullptr);
}

}  // namespace

Server::Server(asio::io_context& io_context) : impl_(new ServerPrivate(io_context)) {}

Server::~Server() { close(); }

void Server::close() {
  if (!is_open()) {
    return;
  }

  std::unique_lock lock(impl_->mutex);

  crypto::Helpers::memzero(impl_->secret_key.data(), impl_->secret_key.size());

  impl_->socket.reset();

  {
    std::unique_lock lock(impl_->connections, std::try_to_lock);

    ASSERT(lock.owns_lock());

    impl_->connections.clear();
  }
  {
    std::unique_lock lock(impl_->pending_connections, std::try_to_lock);

    ASSERT(lock.owns_lock());

    impl_->pending_connections.clear();
  }
}

bool Server::has_pending_connections() const {
  std::shared_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  {
    std::unique_lock lock(impl_->pending_connections);

    return !impl_->pending_connections.empty();
  }
}

bool Server::is_open() const {
  std::shared_lock lock(impl_->mutex);

  if (impl_->socket == nullptr) {
    return false;
  }

  ASSERT(impl_->socket->is_open());

  return true;
}

asio::ip::udp::endpoint Server::local_endpoint() const {
  std::shared_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  return impl_->socket->local_endpoint();
}

std::shared_ptr<Connection> Server::next_pending_connection() {
  std::shared_lock lock(impl_->mutex);

  if (!is_open()) {
    throw std::runtime_error("is not open");
  }

  ConnectionID connection_id;

  {
    std::unique_lock lock(impl_->pending_connections);

    if (impl_->pending_connections.empty()) {
      return nullptr;
    }

    connection_id = impl_->pending_connections.front();

    impl_->pending_connections.pop_front();
  }
  {
    std::shared_lock lock(impl_->connections);

    auto connections_iterator = impl_->connections.find(connection_id);

    if (connections_iterator == impl_->connections.end()) {
      return nullptr;
    }

    auto& connection_details = connections_iterator->second;

    {
      std::unique_lock lock(connection_details);

      ASSERT(connection_details.state == ConnectionDetails::State::Pending);

      connection_details.state = ConnectionDetails::State::Accepted;

      return std::move(connection_details.connection);
    }
  }
}

void Server::open(Configuration&& config) {
  if (config.secret_key.size() != crypto::SIDHp434_compressed::SecretKeyBLength) {
    throw std::runtime_error("secret_key has incorrect size");
  }

  if (is_open()) {
    throw std::runtime_error("is already open");
  }

  asio::ip::udp::socket socket(impl_->io_context);

  socket.close();
  socket.open(config.local_endpoint.protocol());
  socket.bind(config.local_endpoint);

  if (config.receive_buffer_size.has_value()) {
    socket.set_option(asio::socket_base::receive_buffer_size(*config.receive_buffer_size));
  }

  std::unique_lock lock(impl_->mutex);

  impl_->backlog = config.backlog;

  crypto::Helpers::memzero(impl_->secret_key.data(), impl_->secret_key.size());

  impl_->secret_key = std::move(config.secret_key);
  impl_->socket = std::make_shared<asio::ip::udp::socket>(std::move(socket));

  impl_->connections.next_id = std::numeric_limits<ConnectionID>::min();

  async_recursive_read_datagram(
      impl_->io_context, impl_->socket, [weak_impl = impl_->weak_from_this()](auto&&... args) {
        if (auto impl = weak_impl.lock()) [[likely]] {
          impl->async_receive_socket_handler(std::forward<decltype(args)>(args)...);
        }
      });
}

std::shared_ptr<Server::NewConnectionEvent> Server::new_connection() const {
  return impl_->new_connection_event;
}

ServerPrivate::ServerPrivate(asio::io_context& io_context)
    : io_context(io_context), new_connection_event(Server::NewConnectionEvent::create()) {}

ServerPrivate::~ServerPrivate() = default;

void ServerPrivate::create_new_connection(std::vector<uint8_t>&& data,
                                          asio::ip::udp::endpoint&& endpoint) {
  Packet packet(data);

  ChunkList chunk_list(packet.data());

  if (!chunk_list.validate()) [[unlikely]] {
    return;
  }
  if (chunk_list.size() != 1) [[unlikely]] {
    return;
  }

  Chunk chunk(chunk_list.chunk_data(0));

  if (!chunk.validate()) [[unlikely]] {
    return;
  }
  if (chunk.type() != ChunkType::Initiation) [[unlikely]] {
    return;
  }

  if (backlog != 0) {
    std::unique_lock lock(pending_connections);

    if (pending_connections.size() == backlog) {
      return;
    }
  }

  auto [rx_socket, rx_socket_server] = create_socket_pair(io_context);

  if (rx_socket == nullptr || rx_socket_server == nullptr) [[unlikely]] {
    return;
  }

  auto [tx_socket, tx_socket_server] = create_socket_pair(io_context);

  if (tx_socket == nullptr || tx_socket_server == nullptr) [[unlikely]] {
    return;
  }

  ConnectionID connection_id;

  {
    std::unique_lock lock(connections);

    while (true) {
      auto [iterator, success] = connections.try_emplace(connections.next_id++, io_context);

      if (success) [[likely]] {
        connection_id = iterator->first;
        break;
      }
    }
  }
  {
    std::shared_lock lock(connections);

    auto connections_iterator = connections.find(connection_id);

    ASSERT(connections_iterator != connections.end());

    auto& connection_details = connections_iterator->second;

    {
      std::unique_lock lock(connection_details);

      connection_details.connection = std::make_shared<Connection>(io_context);
      connection_details.state_changed_subscription =
          connection_details.connection->state_changed()->subscribe(
              [weak_self = weak_from_this(), connection_id](auto&&... args) {
                if (auto self = weak_self.lock()) [[likely]] {
                  self->state_changed_event_connection_handler(
                      connection_id, std::forward<decltype(args)>(args)...);
                }
              });

      async_recursive_read_datagram(connection_details.strand, tx_socket_server,
                                    [weak_self = weak_from_this(), connection_id](auto&&... args) {
                                      if (auto self = weak_self.lock()) [[likely]] {
                                        self->async_receive_tx_socket_server_handler(
                                            connection_id, std::forward<decltype(args)>(args)...);
                                      }
                                    });

      connection_details.rx_socket_server = rx_socket_server;
      connection_details.tx_socket_server = tx_socket_server;
      connection_details.source_endpoint = std::move(endpoint);

      connection_details.state = ConnectionDetails::State::Connecting;

      Connection::ServerConfiguration config;

      config.rx_socket = std::move(rx_socket);
      config.tx_socket = std::move(tx_socket);
      config.connection_id = connection_id;
      config.secret_key = secret_key;

      connection_details.connection->associate(std::move(config));
    }
  }

  async_send_datagram(*rx_socket_server, std::nullopt, std::move(data));
}

void ServerPrivate::redirect_encrypted_packet(std::vector<uint8_t>&& data,
                                              asio::ip::udp::endpoint&& endpoint) {
  Packet packet(data);

  std::shared_lock lock(connections);

  auto connections_iterator = connections.find(packet.connection_id());

  if (connections_iterator == connections.cend()) [[unlikely]] {
    return;
  }

  auto& connection_details = connections_iterator->second;

  {
    std::unique_lock lock(connection_details);

    if (connection_details.state == ConnectionDetails::State::Closing) [[unlikely]] {
      return;
    }

    async_send_datagram(*connection_details.rx_socket_server, std::nullopt, std::move(data));

    connection_details.source_endpoint = std::move(endpoint);
  }
}

void ServerPrivate::start_closing_timer(ConnectionDetails& connection_details,
                                        ConnectionID connection_id) {
  connection_details.closing_timer.emplace(io_context, CLOSING_INTERVAL);
  connection_details.closing_timer->async_wait(asio::bind_executor(
      connection_details.strand,
      [weak_self = weak_from_this(), connection_id](const asio::error_code& error) {
        if (error) [[unlikely]] {
          return;
        }
        if (auto self = weak_self.lock()) [[likely]] {
          self->async_wait_closing_timer_handler(connection_id);
        }
      }));
}

void ServerPrivate::async_receive_socket_handler(std::vector<uint8_t> data,
                                                 asio::ip::udp::endpoint endpoint) {
  std::shared_lock lock(mutex, std::try_to_lock);

  if (!lock.owns_lock()) {
    return;
  }

  Packet packet(data);

  if (!packet.validate()) [[unlikely]] {
    return;
  }

  if (packet.bits().e) [[likely]] {
    redirect_encrypted_packet(std::move(data), std::move(endpoint));
  } else {
    create_new_connection(std::move(data), std::move(endpoint));
  }
}

void ServerPrivate::async_receive_tx_socket_server_handler(
    ConnectionID connection_id, std::vector<uint8_t> data,
    [[maybe_unused]] const asio::generic::datagram_protocol::endpoint& endpoint) {
  [[maybe_unused]] Packet packet(data);

  ASSERT(packet.validate());
  ASSERT(packet.connection_id() == connection_id);

  std::shared_lock lock(mutex, std::try_to_lock);

  if (!lock.owns_lock()) {
    return;
  }

  {
    std::shared_lock lock(connections);

    auto connections_iterator = connections.find(connection_id);

    ASSERT(connections_iterator != connections.end());

    auto& connection_details = connections_iterator->second;

    {
      std::unique_lock lock(connection_details);

      async_send_datagram(*socket, connection_details.source_endpoint, std::move(data));
    }
  }
}

void ServerPrivate::async_wait_closing_timer_handler(ConnectionID connection_id) {
  std::shared_lock lock(mutex, std::try_to_lock);

  if (!lock.owns_lock()) {
    return;
  }

  {
    std::unique_lock lock(connections);

    auto connections_iterator = connections.find(connection_id);

    ASSERT(connections_iterator != connections.end());

    auto& connection_details = connections_iterator->second;

    {
      std::unique_lock lock(connection_details, std::try_to_lock);

      ASSERT(lock.owns_lock());

      ASSERT(connection_details.state == ConnectionDetails::State::Closing);
    }

    connections.erase(connections_iterator);
  }
}

void ServerPrivate::state_changed_event_connection_handler(ConnectionID connection_id,
                                                           Connection::State new_state) {
  std::shared_lock lock(mutex, std::try_to_lock);

  if (!lock.owns_lock()) {
    return;
  }

  switch (new_state) {
    case Connection::State::Closed: {
      std::shared_lock lock(connections);

      auto connections_iterator = connections.find(connection_id);

      ASSERT(connections_iterator != connections.end());

      auto& connection_details = connections_iterator->second;

      {
        std::unique_lock lock(connection_details);

        if (connection_details.state == ConnectionDetails::State::Pending) {
          std::unique_lock lock(pending_connections);

          pending_connections.erase(connection_details.pending_connections_iterator);
        }

        connection_details.state_changed_subscription.reset();

        connection_details.state = ConnectionDetails::State::Closing;

        start_closing_timer(connection_details, connection_id);
      }
    } break;
    case Connection::State::Established: {
      std::shared_lock lock(connections);

      auto connections_iterator = connections.find(connection_id);

      ASSERT(connections_iterator != connections.end());

      auto& connection_details = connections_iterator->second;

      {
        std::unique_lock lock(connection_details);

        ASSERT(connection_details.state == ConnectionDetails::State::Connecting);

        connection_details.state = ConnectionDetails::State::Pending;

        {
          std::unique_lock lock(pending_connections);

          connection_details.pending_connections_iterator =
              pending_connections.emplace(pending_connections.cend(), connection_id);

          new_connection_event->emit();
        }
      }
    } break;
    default:
      break;
  }
}

}  // namespace protocol
