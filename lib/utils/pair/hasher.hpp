#pragma once

#include <utility>

#include "../hash_combine.hpp"

namespace utils::pair {

struct hasher {
  template <typename T1, typename T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const noexcept {
    size_t seed = 0;
    hash_combine(seed, p.first, p.second);
    return seed;
  }
};

}  // namespace utils::pair
