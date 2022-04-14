#pragma once

#include <cstring>
#include <span>

#include "../debug/assert.hpp"

namespace utils::span {

template <typename T>
void copy(std::span<T> dest, std::span<const T> src) {
  ASSERT(dest.size() == src.size());
  std::memcpy(dest.data(), src.data(), dest.size_bytes());
}

}  // namespace utils::span
