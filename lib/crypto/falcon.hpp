#pragma once

#include <cstdint>
#include <span>

namespace crypto {

struct Falcon512 {
  static constexpr size_t PublicKeyLength = 897;
  static constexpr size_t SecretKeyLength = 1281;
  static constexpr size_t SignatureLength = 666;

  static void generate_keypair(std::span<uint8_t> public_key, std::span<uint8_t> secret_key);

  static void sign(std::span<uint8_t> signature, std::span<const uint8_t> message,
                   std::span<const uint8_t> secret_key);

  static bool verify(std::span<const uint8_t> signature, std::span<const uint8_t> message,
                     std::span<const uint8_t> public_key);
};

}  // namespace crypto
