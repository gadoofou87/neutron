#pragma once

#include <optional>

#include "communication/server.hpp"
#include "protocol/connection.hpp"

namespace detail {

class ClientManager;

class Client : std::enable_shared_from_this<Client> {
 public:
  explicit Client(uint64_t id, std::shared_ptr<protocol::Connection>&& connection);
  Client(const Client&) = delete;
  ~Client();

  [[nodiscard]] const communication::Server& communication() const;

  [[nodiscard]] communication::Server& communication();

  [[nodiscard]] uint32_t device_id() const;

  [[nodiscard]] uint64_t id() const;

  [[nodiscard]] bool is_authorized() const;

  [[nodiscard]] uint64_t user_id() const;

 private:
  void new_raw_data_event_communication_handler();

  void new_request_event_communication_handler();

 private:
  const uint64_t id_;

  const std::shared_ptr<protocol::Connection> connection_;
  std::shared_ptr<protocol::Connection::StateChangedEvent::Subscription>
      state_changed_subscription_;

  communication::Server communication_;
  std::shared_ptr<communication::Server::NewRawDataEvent::Subscription> new_raw_data_subscription_;
  std::shared_ptr<communication::Server::NewRequestEvent::Subscription> new_request_subscription_;

  bool authorized_;
  uint32_t user_id_;
  uint64_t device_id_;

 private:
  friend ClientManager;
};

}  // namespace detail
