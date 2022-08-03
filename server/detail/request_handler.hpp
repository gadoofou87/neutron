#pragma once

#include <cstdint>
#include <string>

namespace api {

class Request;

}

namespace detail {

class Client;

class RequestHandler {
 public:
  void handle(Client& client, size_t request_identifier, const api::Request& request);

 private:
  struct InternalServerError : std::exception {
    InternalServerError() = default;
  };

  struct InvalidDataException : std::exception {
    InvalidDataException() = default;
  };

  struct NotImplementedException : std::exception {
    NotImplementedException() = default;
  };

  struct UnauthorizedException : std::exception {
    UnauthorizedException() = default;
  };

 private:
  struct Helpers;

  template <typename T>
  std::string handle(const T&);

  template <typename T>
  std::string handle(const Client& client, const T&);
};

}  // namespace detail
