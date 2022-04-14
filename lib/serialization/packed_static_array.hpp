#pragma once

#include <cstddef>
#include <span>
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

 private:
  data_type data_[N];
};

}  // namespace serialization
