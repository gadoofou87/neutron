#pragma once

#include <asio/io_context.hpp>

#include "protocol/connection.hpp"
#include "protocol/server.hpp"
#include "server.hpp"

namespace detail {

class ServerPrivate {
 public:
  explicit ServerPrivate();
  ~ServerPrivate();

 public:
  asio::io_context io_context;

  // protocol::Server server;
};

}  // namespace detail
