#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <optional>

#include "detail/connection/api/types/connection_id.hpp"
#include "utils/event.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

}

class Stream;

class Connection {
 public:
  enum class Error {};
  enum class Option {};
  enum class State {
    Closed,
    Listen,
    InitSent,
    InitReceived,
    Established,
    ShutdownPending,
    ShutdownSent,
    ShutdownReceived,
    ShutdownAckSent,
  };
  enum class Type { Client, Server };

  using ReadyReadEvent = utils::Event<size_t /* stream_identifier */>;
  using StateChangedEvent = utils::Event<State /* new_state */>;

 private:
  struct BaseConfiguration {
    std::shared_ptr<asio::generic::datagram_protocol::socket> rx_socket;
    std::shared_ptr<asio::generic::datagram_protocol::socket> tx_socket;
    std::optional<asio::generic::datagram_protocol::endpoint> peer_endpoint;
  };

 public:
  struct ClientConfiguration : BaseConfiguration {
    std::vector<uint8_t> peer_public_key;
  };
  struct ServerConfiguration : BaseConfiguration {
    detail::ConnectionID connection_id;
    std::vector<uint8_t> secret_key;
  };

 public:
  explicit Connection(asio::io_context& io_context);
  ~Connection();

  void abort();

  void associate(ClientConfiguration&& config);

  void associate(ServerConfiguration&& config);

  [[nodiscard]] size_t max_num_streams() const;

  // option

  [[nodiscard]] std::optional<size_t> readable_stream() const;

  void shutdown();

  [[nodiscard]] State state() const;

  [[nodiscard]] Type type() const;

  const Stream& operator[](size_t stream_identifier) const;

  Stream& operator[](size_t stream_identifier);

 public:
  [[nodiscard]] std::shared_ptr<ReadyReadEvent> ready_read() const;

  [[nodiscard]] std::shared_ptr<StateChangedEvent> state_changed() const;

 private:
  const std::shared_ptr<detail::ConnectionPrivate> impl_;
};

}  // namespace protocol
