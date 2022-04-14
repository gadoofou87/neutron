#pragma once

#include "connection.hpp"

namespace protocol {

namespace detail {

struct Events {
 public:
  explicit Events();
  ~Events();

 public:
  std::shared_ptr<Connection::ReadyReadEvent> ready_read;
  std::shared_ptr<Connection::StateChangedEvent> state_changed;
};

}  // namespace detail

}  // namespace protocol
