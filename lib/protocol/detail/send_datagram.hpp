#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <optional>

namespace protocol {

namespace detail {

template <typename DatagramProtocol>
void send_datagram(asio::basic_datagram_socket<DatagramProtocol>& socket,
                   const std::optional<typename DatagramProtocol::endpoint>& endpoint,
                   std::vector<uint8_t> data) {
  asio::const_buffer buffer_view(data.data(), data.size());

  try {
    socket.wait(asio::socket_base::wait_write);

    if (endpoint.has_value()) {
      socket.send_to(buffer_view, *endpoint);
    } else {
      socket.send(buffer_view);
    }
  } catch (const asio::system_error&) {
  }
}

}  // namespace detail

}  // namespace protocol
