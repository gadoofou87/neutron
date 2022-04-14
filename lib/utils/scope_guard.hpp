#pragma once

#include <functional>

namespace utils {

class ScopeGuard {
 public:
  using CleanupFunction = std::function<void()>;

 public:
  ScopeGuard(CleanupFunction function);
  ScopeGuard(const ScopeGuard&) = delete;
  ~ScopeGuard();

  void dismiss();

 private:
  CleanupFunction function_;
};

}  // namespace utils
