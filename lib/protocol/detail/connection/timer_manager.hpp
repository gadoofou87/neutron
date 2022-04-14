#pragma once

#include <array>
#include <asio/steady_timer.hpp>
#include <optional>

#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class TimerManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  enum class TimerId { Init, Shutdown, Rtx, Ack, Heartbeat };

 private:
  static constexpr size_t TIMER_COUNT = 5;

 public:
  using Parentable::Parentable;

  template <TimerId Id>
  [[nodiscard]] bool is_expired() const {
    if (!is_started<Id>()) {
      return false;
    }
    return timers_[timer_index<Id>()]->expiry() < std::chrono::steady_clock::now();
  }

  template <TimerId Id>
  [[nodiscard]] bool is_started() const {
    return timers_[timer_index<Id>()].has_value();
  }

  void reset() override;

  template <TimerId>
  void start();

  template <TimerId Id>
  void stop() {
    timers_[timer_index<Id>()].reset();
  }

  void stop_all();

 private:
  template <TimerId>
  static void async_wait_timer_handler(ConnectionPrivate& parent);

 private:
  template <TimerId>
  bool handler();

  template <TimerId>
  void start_helper(std::chrono::milliseconds expiry_time);

  template <TimerId Id>
  [[nodiscard]] size_t timer_index() const {
    constexpr auto result = std::underlying_type_t<TimerId>(Id);
    // clang-format off
    static_assert(result  < TIMER_COUNT);
    // clang-format on
    return result;
  }

 private:
  std::chrono::milliseconds ack_interval_;
  std::chrono::milliseconds heartbeat_interval_;
  std::array<std::optional<asio::steady_timer>, TIMER_COUNT> timers_;
};

template <>
void TimerManager::start<TimerManager::TimerId::Init>();

template <>
void TimerManager::start<TimerManager::TimerId::Shutdown>();

template <>
void TimerManager::start<TimerManager::TimerId::Rtx>();

template <>
void TimerManager::start<TimerManager::TimerId::Ack>();

template <>
void TimerManager::start<TimerManager::TimerId::Heartbeat>();

}  // namespace detail

}  // namespace protocol
