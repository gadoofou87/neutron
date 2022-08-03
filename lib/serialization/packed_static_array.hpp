#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace serialization {

template <typename T, size_t N>
struct PackedStaticArray {
 public:
  using data_type = std::decay_t<T>;

  static_assert(std::alignment_of_v<data_type> == alignof(char));

 public:
  const data_type *data() const noexcept { return data_; }

  data_type *data() noexcept { return data_; }

  constexpr size_t size() const noexcept { return sizeof(data_); }

  operator std::span<const data_type>() const noexcept { return std::span<const data_type>(data_); }

  operator std::span<data_type>() noexcept { return std::span<data_type>(data_); }

  std::span<const data_type> span() const noexcept { return *this; }

  std::span<data_type> span() noexcept { return *this; }

  operator std::basic_string_view<const data_type>() const noexcept {
    return std::basic_string_view<const data_type>(data_);
  }

  operator std::basic_string_view<data_type>() noexcept {
    return std::basic_string_view<data_type>(data_);
  }

  std::basic_string_view<const data_type> view() const noexcept { return *this; }

  std::basic_string_view<data_type> view() noexcept { return *this; }

 private:
  data_type data_[N];
};

}  // namespace serialization
