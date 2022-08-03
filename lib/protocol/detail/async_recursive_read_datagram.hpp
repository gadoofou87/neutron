#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <optional>

#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

template <typename Executor, typename DatagramProtocol>
void async_recursive_read_datagram(
    Executor& executor,
    const std::shared_ptr<asio::basic_datagram_socket<DatagramProtocol>>& socket,
    const std::function<void(std::vector<uint8_t>, typename DatagramProtocol::endpoint)>&
        handler_ex) {
  ASSERT(socket != nullptr);
  ASSERT(handler_ex != nullptr);

  auto buffer = std::make_shared<std::vector<uint8_t>>(std::numeric_limits<uint16_t>::max());
  auto endpoint = std::make_shared<typename DatagramProtocol::endpoint>();

  asio::mutable_buffer buffer_view(buffer->data(), buffer->size());

  auto handler = [&executor, weak_socket = std::weak_ptr(socket), buffer, buffer_view, endpoint,
                  handler_ex](auto&& self, const asio::error_code& error,
                              size_t bytes_transferred) {
    if (error) [[unlikely]] {
      if (error == asio::error::operation_aborted) {
        return;
      }
    } else {
      std::vector<uint8_t> data(bytes_transferred);
      std::memcpy(data.data(), buffer->data(), data.size());

      asio::post(executor,
                 [handler_ex, data = std::move(data), endpoint = std::move(*endpoint)]() mutable {
                   return handler_ex(std::move(data), std::move(endpoint));
                 });
    }

    if (auto socket = weak_socket.lock()) [[likely]] {
      socket->async_receive_from(
          buffer_view, *endpoint, [self = std::move(self)](auto&& PH1, auto&& PH2) {
            return self(self, std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
          });
    }
  };

  socket->async_receive_from(
      buffer_view, *endpoint, [handler = std::move(handler)](auto&& PH1, auto&& PH2) {
        return handler(handler, std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
      });
}

}  // namespace detail

}  // namespace protocol
