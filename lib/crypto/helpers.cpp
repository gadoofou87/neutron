#include "helpers.hpp"

#include <cstring>

extern "C" {
#include <randombytes.h>
}

#ifndef UINT64_WIDTH
#define UINT64_WIDTH 64
#endif

namespace crypto {

bool Helpers::memcmp(const void* a, const void* b, size_t len) {
  uint8_t r = 0;

  for (size_t i = 0; i < len; ++i) {
    r |= reinterpret_cast<const uint8_t*>(a)[i] ^ reinterpret_cast<const uint8_t*>(b)[i];
  }

  return (-static_cast<uint64_t>(r)) >> (UINT64_WIDTH - 1) == 0;
}

void Helpers::memzero(void* ptr, size_t len) {
  using memset_t = void* (*)(void*, int, size_t);
  static volatile memset_t memset_func = memset;
  memset_func(ptr, 0, len);
}

void Helpers::randombytes_buf(void* buf, size_t size) {
  while (randombytes(reinterpret_cast<uint8_t*>(buf), size) != 0) {
  }
}

}  // namespace crypto
