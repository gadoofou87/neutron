#pragma once

#include <cstddef>

namespace crypto {

struct Helpers {
  [[nodiscard]] static bool memcmp(const void* a, const void* b, size_t len);

  static void memzero(void* ptr, size_t len);

  static void randombytes_buf(void* buf, size_t size);
};

}  // namespace crypto
