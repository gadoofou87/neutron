#pragma once

#include <cstring>
#include <span>

namespace utils::span {

template <typename T>
struct equal_to {
  using is_transparent = void;

  bool operator()(std::span<const T> x, std::span<const T> y) const {
    if (x.size_bytes() != y.size_bytes()) {
      return false;
    }
    if (std::memcmp(x.data(), y.data(), x.size_bytes()) != 0) {
      return false;
    }
    return true;
  }
};

}  // namespace utils::span
