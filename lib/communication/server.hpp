#pragma once

#include <optional>
#include <span>

#include "detail/base.hpp"
#include "utils/event.hpp"

namespace communication {

namespace detail {

class ServerPrivate;

}

class Server : public detail::Base {
 public:
  using NewRequestEvent = utils::Event<>;

 public:
  explicit Server();
  ~Server() override;

  [[nodiscard]] bool has_pending_requests() const;

  [[nodiscard]] size_t max_exception_code() const;

  std::optional<std::pair<size_t, std::vector<uint8_t>>> next_pending_request();

  void send_event(std::span<const uint8_t> data);

  template <typename T>
  auto send_exception(size_t request_id, T code) requires(std::is_enum_v<T>) {
    return send_exception(request_id, static_cast<size_t>(code));
  }

  void send_exception(size_t request_id, size_t code);

  void send_response(size_t request_id, std::span<const uint8_t> data);

 public:
  [[nodiscard]] std::shared_ptr<NewRequestEvent> new_request() const;

 private:
  detail::ServerPrivate* const impl_;
};

}  // namespace communication
