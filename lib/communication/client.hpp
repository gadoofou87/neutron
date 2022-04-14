#pragma once

#include <functional>
#include <optional>
#include <span>

#include "detail/base.hpp"
#include "utils/event.hpp"

namespace communication {

namespace detail {

class ClientPrivate;

}

class Client : public detail::Base {
 public:
  using NewEventEvent = utils::Event<>;
  using ExceptionCallback = std::function<void(size_t /* code */)>;
  using ResponseCallback = std::function<void(std::vector<uint8_t> /* data */)>;

 public:
  explicit Client();
  ~Client() override;

  [[nodiscard]] bool has_pending_events() const;

  std::optional<std::vector<uint8_t>> next_pending_event();

  void send_request(std::span<const uint8_t> data, const ResponseCallback& on_response,
                    const ExceptionCallback& on_exception = nullptr);

 public:
  [[nodiscard]] std::shared_ptr<NewEventEvent> new_event() const;

 private:
  detail::ClientPrivate* const impl_;
};

}  // namespace communication
