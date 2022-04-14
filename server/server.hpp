#pragma once

#include <memory>
#include <string_view>

namespace detail {

class ServerPrivate;

}

class Server {
 public:
  explicit Server();
  ~Server();

  void start(std::string_view config_path);

  void stop();

 private:
  const std::unique_ptr<detail::ServerPrivate> impl_;
};
