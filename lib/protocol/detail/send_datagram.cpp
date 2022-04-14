#include "send_datagram.hpp"

namespace protocol {

namespace detail {

void send_datagram(asio::generic::datagram_protocol::socket& socket,
                   const std::optional<asio::generic::datagram_protocol::endpoint>& endpoint,
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
