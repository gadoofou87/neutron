#pragma once

#include <cstddef>
#include <memory>

namespace protocol {

class Connection;

}

namespace communication {

namespace detail {

class BasePrivate;

class Base {
 public:
  explicit Base(std::shared_ptr<BasePrivate> impl);
  virtual ~Base();

  void close();

  [[nodiscard]] size_t current_read_stream() const;

  [[nodiscard]] size_t current_write_stream() const;

  [[nodiscard]] bool is_open() const;

  void open(std::shared_ptr<protocol::Connection> connection);

  void set_current_read_stream(size_t stream_identifier);

  void set_current_write_stream(size_t stream_identifier);

 protected:
  const std::shared_ptr<BasePrivate> impl_;
};

}  // namespace detail

}  // namespace communication
