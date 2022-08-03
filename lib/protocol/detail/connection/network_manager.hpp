#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <list>
#include <memory>
#include <optional>

#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class NetworkManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  [[nodiscard]] bool is_receiving() const;

  void reset() override;

  void set_rx_socket(std::shared_ptr<asio::generic::datagram_protocol::socket> socket);

  void set_tx_endpoint(std::optional<asio::generic::datagram_protocol::endpoint> endpoint);

  void set_tx_socket(std::shared_ptr<asio::generic::datagram_protocol::socket> socket);

  void start_receive();

  void stop_receive();

  template <bool Async = true>
  void write_pending_packets();

 private:
  static void async_receive_rx_socket_handler(ConnectionPrivate& parent, std::vector<uint8_t> data,
                                              asio::generic::datagram_protocol::endpoint endpoint);

 private:
  std::list<std::vector<uint8_t>> gather_outbound();

  void write(std::vector<uint8_t> data, bool async);

 private:
  bool receiving_;
  std::shared_ptr<asio::generic::datagram_protocol::socket> rx_socket_;
  std::optional<asio::generic::datagram_protocol::endpoint> tx_endpoint_;
  std::shared_ptr<asio::generic::datagram_protocol::socket> tx_socket_;
};

}  // namespace detail

}  // namespace protocol
