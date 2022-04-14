#include "async_send_datagram.hpp"

#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

void async_send_datagram(asio::generic::datagram_protocol::socket& socket,
                         const std::optional<asio::generic::datagram_protocol::endpoint>& endpoint,
                         std::vector<uint8_t> data) {
  auto buffer = std::make_shared<decltype(data)>(std::move(data));

  asio::const_buffer buffer_view(buffer->data(), buffer->size());

  auto handler = [buffer](const asio::error_code& error,
                          [[maybe_unused]] size_t bytes_transferred) {
    if (!error) {
      ASSERT(bytes_transferred == buffer->size());
    }
  };

  if (endpoint.has_value()) {
    socket.async_send_to(buffer_view, *endpoint, std::move(handler));
  } else {
    socket.async_send(buffer_view, std::move(handler));
  }
}

}  // namespace detail

}  // namespace protocol
