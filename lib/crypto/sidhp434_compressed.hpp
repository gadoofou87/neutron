#pragma once

#include <cstdint>
#include <span>

namespace crypto {

struct SIDHp434_compressed {
  static constexpr size_t PublicKeyLength = 197;
  static constexpr size_t SecretKeyALength = 27;
  static constexpr size_t SecretKeyBLength = 28;
  static constexpr size_t SharedSecretLength = 110;

  static void generate_keypair_A(std::span<uint8_t> public_key, std::span<uint8_t> secret_key);

  static void generate_keypair_B(std::span<uint8_t> public_key, std::span<uint8_t> secret_key);

  static void agree_A(std::span<uint8_t> shared_secret, std::span<const uint8_t> secret_key_A,
                      std::span<const uint8_t> public_key_B);

  static void agree_B(std::span<uint8_t> shared_secret, std::span<const uint8_t> secret_key_B,
                      std::span<const uint8_t> public_key_A);
};

}  // namespace crypto
