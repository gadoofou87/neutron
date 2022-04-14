#include "scope_guard.hpp"

namespace utils {

ScopeGuard::ScopeGuard(CleanupFunction function) : function_(std::move(function)) {}

ScopeGuard::~ScopeGuard() {
  if (function_ != nullptr) {
    function_();
  }
}

void ScopeGuard::dismiss() { function_ = nullptr; }

}  // namespace utils
