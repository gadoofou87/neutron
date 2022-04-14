#include <toml++/toml.h>

#include "server_p.hpp"

using namespace detail;

Server::Server() : impl_(new ServerPrivate) {}

Server::~Server() { stop(); }

void Server::start(std::string_view config_path) {
  auto config = toml::parse_file(config_path);

  // num_threads

  // address
  // port
  // backlog
  // receive_buffer_size ?

  //
}

void Server::stop() {
  //
}

ServerPrivate::ServerPrivate() = default;

ServerPrivate::~ServerPrivate() = default;
