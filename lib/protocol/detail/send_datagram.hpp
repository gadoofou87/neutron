#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <optional>

namespace protocol {

namespace detail {

void send_datagram(asio::generic::datagram_protocol::socket& socket,
                   const std::optional<asio::generic::datagram_protocol::endpoint>& endpoint,
                   std::vector<uint8_t> data);

}

}  // namespace protocol
