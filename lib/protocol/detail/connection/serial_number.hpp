#pragma once

#include <limits>
#include <type_traits>

namespace protocol {

namespace detail {

template <typename T>
class SerialNumber {
 public:
  using value_type = std::make_unsigned_t<T>;

 private:
  static constexpr auto SERIAL_BITS = std::numeric_limits<T>::digits - 1;
  static constexpr auto MAX = static_cast<value_type>(1) << SERIAL_BITS;

 public:
  SerialNumber() = delete;

  struct EqualTo {
    constexpr bool operator()(value_type lhs, value_type rhs) const { return lhs == rhs; }
  };
  struct NotEqualTo {
    constexpr bool operator()(value_type lhs, value_type rhs) const { return lhs != rhs; }
  };
  struct Less {
    constexpr bool operator()(value_type lhs, value_type rhs) const {
      return (lhs < rhs && rhs - lhs <= MAX) || (lhs > rhs && lhs - rhs >= MAX);
    }
  };
  struct Greater {
    constexpr bool operator()(value_type lhs, value_type rhs) const {
      return (lhs < rhs && rhs - lhs >= MAX) || (lhs > rhs && lhs - rhs <= MAX);
    }
  };
  struct LessEqual {
    constexpr bool operator()(value_type lhs, value_type rhs) const {
      return EqualTo{}(lhs, rhs) || Less{}(lhs, rhs);
    }
  };
  struct GreaterEqual {
    constexpr bool operator()(value_type lhs, value_type rhs) const {
      return EqualTo{}(lhs, rhs) || Greater{}(lhs, rhs);
    }
  };
};

}  // namespace detail

}  // namespace protocol
