#pragma once

#include <deque>
#include <mutex>
#include <span>

#include "base.hpp"
#include "detail/api/structures/message.hpp"
#include "detail/api/structures/raw_data.hpp"
#include "protocol/connection.hpp"

namespace communication {

namespace detail {

class BasePrivate : public std::enable_shared_from_this<BasePrivate> {
 public:
  explicit BasePrivate();
  virtual ~BasePrivate();

  void handle(RawData raw_data);

  void send_message(size_t stream_identifier, MessageType type, std::vector<uint8_t>&& data);

 public:
  void ready_read_handler(size_t stream_identifier);

  virtual void message_handler(size_t stream_identifier, Message message) = 0;

 public:
  std::recursive_mutex mutex;

  std::shared_ptr<protocol::Connection> connection;
  std::shared_ptr<protocol::Connection::ReadyReadEvent::Subscription> ready_read_subscription;

 private:
  std::shared_ptr<Base::NewRawDataEvent> new_raw_data_event;

  std::deque<std::vector<uint8_t>> pending_raw_data;

  friend Base;
};

}  // namespace detail

}  // namespace communication
