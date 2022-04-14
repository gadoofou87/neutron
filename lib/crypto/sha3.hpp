#pragma once

#include <cstdint>
#include <span>

namespace crypto {

struct SHAKE128 {
  static void hash(std::span<uint8_t> output, std::span<const uint8_t> input);
};

struct SHAKE256 {
  static void hash(std::span<uint8_t> output, std::span<const uint8_t> input);
};

struct SHA3_256 {
  static constexpr size_t DigestSize = 32;

  static void hash(std::span<uint8_t> output, std::span<const uint8_t> input);
};

struct SHA3_384 {
  static constexpr size_t DigestSize = 48;

  static void hash(std::span<uint8_t> output, std::span<const uint8_t> input);
};

struct SHA3_512 {
  static constexpr size_t DigestSize = 64;

  static void hash(std::span<uint8_t> output, std::span<const uint8_t> input);
};

template <typename T>
concept Sha3Concept = std::is_same_v<T, SHAKE128> || std::is_same_v<T, SHAKE256> ||
    std::is_same_v<T, SHA3_256> || std::is_same_v<T, SHA3_384> || std::is_same_v<T, SHA3_512>;

}  // namespace crypto
