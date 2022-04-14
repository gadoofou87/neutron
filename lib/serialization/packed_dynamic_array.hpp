#pragma once

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

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

 private:
  size_type size_;
  data_type data_[1];  // UB ?
};

}  // namespace serialization
