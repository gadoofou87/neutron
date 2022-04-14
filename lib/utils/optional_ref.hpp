#pragma once

#include <optional>

#include "debug/assert.hpp"

namespace utils {

template <typename T>
class optional_ref {
 public:
  optional_ref() noexcept : ptr(nullptr) {}

  optional_ref(std::nullopt_t /*unused*/) noexcept : optional_ref() {}

  optional_ref(T& ref) noexcept : ptr(&ref) {}

  [[nodiscard]] bool has_value() const noexcept { return ptr != nullptr; }

  explicit operator bool() const noexcept { return has_value(); }

  const T* operator->() const noexcept {
    check_bad_optional_access();
    return ptr;
  }

  T* operator->() noexcept {
    check_bad_optional_access();
    return ptr;
  }

  const T& operator*() const noexcept {
    check_bad_optional_access();
    return *ptr;
  }

  T& operator*() noexcept {
    check_bad_optional_access();
    return *ptr;
  }

 private:
  void check_bad_optional_access() const noexcept {
    ASSERT_X(ptr != nullptr, "bad optional access");
  }

 private:
  T* ptr;
};

}  // namespace utils
