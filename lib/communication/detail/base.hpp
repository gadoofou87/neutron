#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>

#include "utils/event.hpp"

namespace protocol {

class Connection;

}

namespace communication {

namespace detail {

class BasePrivate;

class Base {
 public:
  using NewRawDataEvent = utils::Event<>;

 public:
  explicit Base(std::shared_ptr<BasePrivate> impl);
  virtual ~Base();

  void close();

  [[nodiscard]] bool is_open() const;

  [[nodiscard]] bool has_pending_raw_data() const;

  std::optional<std::vector<uint8_t>> next_pending_raw_data();

  void open(std::shared_ptr<protocol::Connection> connection);

  void send_raw_data(size_t stream_identifier, std::span<const uint8_t> data);

 public:
  [[nodiscard]] std::shared_ptr<NewRawDataEvent> new_raw_data() const;

 protected:
  const std::shared_ptr<BasePrivate> impl_;
};

}  // namespace detail

}  // namespace communication
