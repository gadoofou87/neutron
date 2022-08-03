#pragma once

#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace serialization {

template <typename T, typename U>
struct PackedDynamicArray {
 private:
  using self_type = PackedDynamicArray<T, U>;

 public:
  using data_type = std::decay_t<T>;
  using size_type = std::decay_t<U>;

  static_assert(std::alignment_of_v<data_type> == alignof(char));
  static_assert(std::alignment_of_v<size_type> == alignof(char));

 public:
  const data_type *data() const noexcept { return data_; }

  data_type *data() noexcept { return data_; }

  size_type &size() noexcept { return size_; }

  constexpr auto data_offset() const noexcept { return offsetof(self_type, data_); }

  constexpr auto size_offset() const noexcept { return offsetof(self_type, size_); }

  operator std::span<const data_type>() const noexcept {
    return std::span<const data_type>(data_, size_);
  }

  operator std::span<data_type>() noexcept { return std::span<data_type>(data_, size_); }

  std::span<const data_type> span() const noexcept { return *this; }

  std::span<data_type> span() noexcept { return *this; }

  operator std::basic_string_view<const data_type>() const noexcept {
    return std::basic_string_view<const data_type>(data_, size_);
  }

  operator std::basic_string_view<data_type>() noexcept {
    return std::basic_string_view<data_type>(data_, size_);
  }

  std::basic_string_view<const data_type> view() const noexcept { return *this; }

  std::basic_string_view<data_type> view() noexcept { return *this; }

 private:
  size_type size_;
  data_type data_[1];  // UB ?
};

}  // namespace serialization
