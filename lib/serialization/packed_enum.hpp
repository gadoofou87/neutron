#pragma once

#include <type_traits>

#include "packed_integer.hpp"

namespace serialization {

template <typename T>
requires(std::is_enum_v<T>) struct PackedEnum {
 private:
  using underlying_type = std::underlying_type_t<T>;

 public:
  using data_type = PackedInteger<underlying_type>;

 public:
  operator T() const noexcept { return get(); }

  auto& operator=(T value) noexcept { return set(value), *this; }

 private:
  T get() const noexcept { return static_cast<T>(static_cast<underlying_type>(data_)); }

  void set(T value) noexcept { data_ = static_cast<underlying_type>(value); }

 private:
  data_type data_;
};

}  // namespace serialization
