#pragma once

#include <span>

#include "../hash_range.hpp"

namespace utils::span {

template <typename T>
struct hasher {
  using is_transparent = void;

  std::size_t operator()(std::span<const T> s) const noexcept {
    size_t seed = 0;
    hash_range(seed, s.begin(), s.end());
    return seed;
  }
};

}  // namespace utils::span
