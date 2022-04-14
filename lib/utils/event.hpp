#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

#include "debug/assert.hpp"

namespace utils {

template <typename... Args>
class Event : public std::enable_shared_from_this<Event<Args...>> {
 public:
  class Subscription;

 public:
  [[nodiscard]] static auto create() { return std::shared_ptr<Event>(new Event()); }

 public:
  void emit(const Args&... args) {
    std::unique_lock lock(mutex_);
    for (auto subscriptions = subscriptions_; auto& weak_subscription : subscriptions) {
      if (auto subscription = weak_subscription.lock()) {
        subscription->execute(args...);
      }
    }
  }

  [[nodiscard]] auto subscribe(std::function<void(Args...)> method) {
    std::unique_lock lock(mutex_);
    auto iterator = subscriptions_.emplace(subscriptions_.cend());
    std::shared_ptr<Subscription> subscription(
        new Subscription(std::move(method)),
        [weak_this = this->weak_from_this(), iterator](auto* p) {
          if (auto shared_this = weak_this.lock()) {
            std::unique_lock lock(shared_this->mutex_);
            shared_this->subscriptions_.erase(iterator);
          }
          delete p;
        });
    *iterator = subscription;
    return subscription;
  }

 private:
  Event() = default;

 private:
  mutable std::recursive_mutex mutex_;
  std::list<std::weak_ptr<Subscription>> subscriptions_;
};

template <typename... Args>
class Event<Args...>::Subscription {
 private:
  explicit Subscription(std::function<void(Args...)> method) : method_(std::move(method)) {
    ASSERT(method_ != nullptr);
  }

 public:
  void wait() const {
    std::mutex mutex;
    std::unique_lock lock(mutex);
    cv_.wait(lock);
  }

  bool wait(std::chrono::milliseconds rel_time) const {
    std::mutex mutex;
    std::unique_lock lock(mutex);
    return cv_.wait_for(lock, rel_time) == std::cv_status::no_timeout;
  }

 private:
  void execute(const Args&... args) {
    method_(args...);
    cv_.notify_all();
  }

 private:
  mutable std::condition_variable cv_;
  std::function<void(Args...)> method_;

 private:
  friend Event;
};

}  // namespace utils
