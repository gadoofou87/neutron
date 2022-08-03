#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <optional>

#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

template <typename DatagramProtocol>
void async_send_datagram(asio::basic_datagram_socket<DatagramProtocol>& socket,
                         const std::optional<typename DatagramProtocol::endpoint>& endpoint,
                         std::vector<uint8_t> data) {
  auto buffer = std::make_shared<decltype(data)>(std::move(data));

  asio::const_buffer buffer_view(buffer->data(), buffer->size());

  auto handler = [buffer](const asio::error_code& error,
                          [[maybe_unused]] size_t bytes_transferred) {
    if (!error) [[likely]] {
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
