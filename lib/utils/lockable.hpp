#pragma once

#include <mutex>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace utils {

template <typename, bool>
class Lockable;

template <typename>
struct is_lockable : std::false_type {};

template <typename Object, bool Recursive>
struct is_lockable<Lockable<Object, Recursive>> : std::true_type {};

template <typename Object, bool Recursive = true>
class Lockable : public std::conditional_t<Recursive, std::recursive_mutex, std::mutex> {
 public:
  using object_type = Object;

 public:
  template <typename... Args>
  explicit Lockable(Args&&... args) : object_(std::forward<Args>(args)...) {}
  virtual ~Lockable() = default;

 private:
  Object object_;

 private:
  template <typename Lockable>
  requires(is_lockable<Lockable>::value) friend struct LockedObject;
};

template <typename Lockable>
requires(is_lockable<Lockable>::value) struct LockedObject {
 public:
  explicit LockedObject(Lockable& lockable)
      : lockable_(lockable), lock_(lockable, std::adopt_lock) {}
  virtual ~LockedObject() = default;

  const typename Lockable::object_type* operator->() const noexcept { return &lockable_.object_; }

  typename Lockable::object_type* operator->() noexcept { return &lockable_.object_; }

  const typename Lockable::object_type& operator*() const noexcept { return lockable_.object_; }

  typename Lockable::object_type& operator*() noexcept { return lockable_.object_; }

 private:
  Lockable& lockable_;
  std::unique_lock<Lockable> lock_;
};

template <typename Lockable1, typename... LockableN>
[[nodiscard]] auto lock(Lockable1& lockable1,
                        LockableN&... lockablen) requires(is_lockable<Lockable1>::value &&
                                                          (is_lockable<LockableN>::value && ...)) {
  if constexpr (sizeof...(lockablen) == 0) {
    lockable1.lock();
  } else {
    std::lock(lockable1, lockablen...);
  }
  return std::tuple<LockedObject<Lockable1>, LockedObject<LockableN>...>(lockable1, lockablen...);
}

#if 0
class LockableTryLockException : public std::runtime_error {
 public:
  LockableTryLockException() : std::runtime_error(""){};
};

template <typename Lockable1, typename... LockableN>
[[nodiscard]] auto try_lock(
    Lockable1& lockable1,
    LockableN&... lockablen) requires(is_lockable<Lockable1>::value &&
                                      (is_lockable<LockableN>::value && ...)) {
  bool success;

  if constexpr (sizeof...(lockablen) == 0) {
    success = lockable1.try_lock();
  } else {
    success = std::try_lock(lockable1, lockablen...) == -1;
  }

  if (!success) {
    throw LockableTryLockException();
  }

  return std::tuple<LockedObject<Lockable1>, LockedObject<LockableN>...>(
      lockable1, lockablen...);
}
#endif

}  // namespace utils
