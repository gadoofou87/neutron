#pragma once

#include <asio/ip/udp.hpp>
#include <memory>
#include <optional>

#include "utils/event.hpp"

namespace protocol {

namespace detail {

class ServerPrivate;

}

class Connection;

class Server {
 public:
  using NewConnectionEvent = utils::Event<>;

 public:
  struct Configuration {
    size_t backlog;
    asio::ip::udp::endpoint local_endpoint;
    std::optional<size_t> receive_buffer_size;
    std::vector<uint8_t> secret_key;
  };

 public:
  explicit Server(asio::io_context& io_context);
  ~Server();

  void close();

  [[nodiscard]] bool has_pending_connections() const;

  [[nodiscard]] bool is_open() const;

  [[nodiscard]] asio::ip::udp::endpoint local_endpoint() const;

  std::shared_ptr<Connection> next_pending_connection();

  void open(Configuration&& config);

 public:
  [[nodiscard]] std::shared_ptr<NewConnectionEvent> new_connection() const;

 private:
  const std::shared_ptr<detail::ServerPrivate> impl_;
};

}  // namespace protocol
