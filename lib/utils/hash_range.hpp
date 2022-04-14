#pragma once

#include "hash_combine.hpp"

namespace utils {

template <typename It>
void hash_range(std::size_t& seed, It first, It last) noexcept {
  for (; first != last; ++first) {
    hash_combine(seed, *first);
  }
}

}  // namespace utils
