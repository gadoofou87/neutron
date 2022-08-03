#pragma once

#include <string>

namespace pqxx {

class connection;

}

namespace detail {

class DatabaseManager {
 public:
  pqxx::connection& connection();

  [[nodiscard]] const std::string& connection_string() const;

  void create_tables();

  void set_connection_string(std::string string);

 private:
  std::string connection_string_;
};

}  // namespace detail
