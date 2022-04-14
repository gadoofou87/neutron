#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace serialization {

class PackedStruct {
 public:
  PackedStruct(std::span<uint8_t> raw) : raw_data_(raw.data()), raw_size_(raw.size()) {}

  PackedStruct(uint8_t *raw_data, size_t raw_size) : raw_data_(raw_data), raw_size_(raw_size) {}

  const void *raw_data() const noexcept { return raw_data_; }

  void *raw_data() noexcept { return raw_data_; }

  size_t raw_size() const noexcept { return raw_size_; }

  virtual bool validate() = 0;

 protected:
  bool range_check(size_t offset, size_t size) const noexcept { return offset + size <= raw_size_; }

  template <typename T>
  T *jmp_ptr(size_t offset) noexcept {
    static_assert(std::alignment_of_v<T> == 1);
    static_assert(std::is_standard_layout_v<T>);
    return reinterpret_cast<T *>(jmp(offset));
  }

  template <typename T>
  T &jmp_ref(size_t offset) noexcept {
    return *jmp_ptr<T>(offset);
  }

 private:
  void *jmp(size_t offset) noexcept { return reinterpret_cast<uint8_t *>(raw_data_) + offset; }

 private:
  void *raw_data_;
  size_t raw_size_;
};

}  // namespace serialization
