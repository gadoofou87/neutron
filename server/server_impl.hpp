#pragma once

#include <asio/io_context.hpp>

#include "detail/client_manager.hpp"
#include "detail/database_manager.hpp"
#include "detail/network_manager.hpp"
#include "detail/path_manager.hpp"
#include "detail/request_handler.hpp"
#include "server.hpp"
#include "utils/singleton.hpp"

namespace detail {

class ServerImpl : public utils::Singleton<ServerImpl> {
 public:
  explicit ServerImpl(Singleton::Token);
  virtual ~ServerImpl();

 public:
  asio::io_context io_context;

  ClientManager client_manager;
  DatabaseManager database_manager;
  NetworkManager network_manager;
  PathManager path_manager;
  RequestHandler request_handler;
};

}  // namespace detail
