#pragma once

#include <cstdint>
#include <span>

namespace crypto {

struct ChaCha20Poly1305 {
  static constexpr size_t KeyLength = 32;
  static constexpr size_t IVSize = 12;
  static constexpr size_t DigestSize = 16;

  static void encrypt(std::span<uint8_t> ciphertext, std::span<uint8_t> mac,
                      std::span<const uint8_t> iv, std::span<const uint8_t> key,
                      std::span<const uint8_t> ad, std::span<const uint8_t> message);

  [[nodiscard]] static bool decrypt(std::span<uint8_t> message, std::span<const uint8_t> mac,
                                    std::span<const uint8_t> iv, std::span<const uint8_t> key,
                                    std::span<const uint8_t> ad,
                                    std::span<const uint8_t> ciphertext);
};

struct XChaCha20Poly1305 {
  static constexpr size_t KeyLength = 32;
  static constexpr size_t IVSize = 24;
  static constexpr size_t DigestSize = 16;

  static void encrypt(std::span<uint8_t> ciphertext, std::span<uint8_t> mac,
                      std::span<const uint8_t> iv, std::span<const uint8_t> key,
                      std::span<const uint8_t> ad, std::span<const uint8_t> message);

  [[nodiscard]] static bool decrypt(std::span<uint8_t> message, std::span<const uint8_t> mac,
                                    std::span<const uint8_t> iv, std::span<const uint8_t> key,
                                    std::span<const uint8_t> ad,
                                    std::span<const uint8_t> ciphertext);
};

}  // namespace crypto
