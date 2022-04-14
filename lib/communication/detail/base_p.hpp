#pragma once

#include <mutex>
#include <span>

#include "base.hpp"
#include "detail/api/structures/message.hpp"
#include "protocol/connection.hpp"

namespace communication {

namespace detail {

class BasePrivate : public std::enable_shared_from_this<BasePrivate> {
 public:
  explicit BasePrivate();
  virtual ~BasePrivate();

  void send_message(MessageType type, std::vector<uint8_t>&& data);

 public:
  void ready_read_handler(size_t stream_identifier);

  virtual void message_handler(Message message) = 0;

 public:
  std::recursive_mutex mutex;

  std::shared_ptr<protocol::Connection> connection;
  std::shared_ptr<protocol::Connection::ReadyReadEvent::Subscription> ready_read_subscription;
  size_t current_read_stream;
  size_t current_write_stream;
};

}  // namespace detail

}  // namespace communication
