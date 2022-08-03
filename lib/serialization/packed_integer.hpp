#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace serialization {

template <typename T>
struct PackedInteger {
 public:
  using underlying_type = std::decay_t<T>;

 public:
  PackedInteger() = default;
  PackedInteger(underlying_type value) { set(value); }

  const auto *data() const noexcept { return data_; }

  auto *data() noexcept { return data_; }

  auto size() const noexcept { return sizeof(data_); }

  operator underlying_type() const noexcept { return get(); }

  auto &operator=(underlying_type value) noexcept { return set(value), *this; }

  underlying_type value() const noexcept { return *this; }

 private:
  template <size_t I = 0>
  constexpr underlying_type get() const noexcept {
    if constexpr (I == sizeof(underlying_type)) {
      return 0;
    } else {
      return (static_cast<underlying_type>(data_[I]) << 8 * I) | get<I + 1>();
    }
  }

  template <size_t I = 0>
  constexpr void set(underlying_type value) noexcept {
    if constexpr (I == sizeof(underlying_type)) {
      return;
    } else {
      data_[I] = (value >> 8 * I) & 0xFF;
      set<I + 1>(value);
    }
  }

 private:
  uint8_t data_[sizeof(underlying_type)];
};

using pu8 = PackedInteger<uint8_t>;
using pu16 = PackedInteger<uint16_t>;
using pu32 = PackedInteger<uint32_t>;
using pu64 = PackedInteger<uint64_t>;

using pi8 = PackedInteger<int8_t>;
using pi16 = PackedInteger<int16_t>;
using pi32 = PackedInteger<int32_t>;
using pi64 = PackedInteger<int64_t>;

}  // namespace serialization

namespace std {

template <typename T>
struct numeric_limits<serialization::PackedInteger<T>>
    : public numeric_limits<typename serialization::PackedInteger<T>::underlying_type> {};

}  // namespace std
