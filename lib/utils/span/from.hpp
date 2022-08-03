#pragma once

#include <span>
#include <string>

namespace utils::span {

template <typename T>
std::span<T> from(const std::string& string) {
  return std::span<T>(reinterpret_cast<T*>(string.data()), string.size());
}

}  // namespace utils::span
