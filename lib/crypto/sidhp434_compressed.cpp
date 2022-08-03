#include "sidhp434_compressed.hpp"

#include <fmt/core.h>

extern "C" {
#include <P434_compressed_api.h>
}

namespace crypto {

void SIDHp434_compressed::generate_keypair_A(std::span<uint8_t> public_key,
                                             std::span<uint8_t> secret_key) {
  if (public_key.size() != PublicKeyLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect public key size (actual: {}, expected: {})",
                                         public_key.size(), PublicKeyLength));
  }
  if (secret_key.size() != SecretKeyALength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect secret key size (actual: {}, expected: {})",
                                         secret_key.size(), SecretKeyALength));
  }

  random_mod_order_A_SIDHp434(secret_key.data());

  EphemeralKeyGeneration_A_SIDHp434_Compressed(secret_key.data(), public_key.data());
}

void SIDHp434_compressed::generate_keypair_B(std::span<uint8_t> public_key,
                                             std::span<uint8_t> secret_key) {
  if (public_key.size() != PublicKeyLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect public key size (actual: {}, expected: {})",
                                         public_key.size(), PublicKeyLength));
  }
  if (secret_key.size() != SecretKeyBLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect secret key size (actual: {}, expected: {})",
                                         secret_key.size(), SecretKeyBLength));
  }

  random_mod_order_B_SIDHp434(secret_key.data());

  EphemeralKeyGeneration_B_SIDHp434_Compressed(secret_key.data(), public_key.data());
}

void SIDHp434_compressed::agree_A(std::span<uint8_t> shared_secret,
                                  std::span<const uint8_t> secret_key_A,
                                  std::span<const uint8_t> public_key_B) {
  if (shared_secret.size() != SharedSecretLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect shared secret size (actual: {}, expected: {})",
                                         shared_secret.size(), SharedSecretLength));
  }
  if (secret_key_A.size() != SecretKeyALength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect secret key A size (actual: {}, expected: {})",
                                         secret_key_A.size(), SecretKeyALength));
  }
  if (public_key_B.size() != PublicKeyLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect public key B size (actual: {}, expected: {})",
                                         public_key_B.size(), PublicKeyLength));
  }

  EphemeralSecretAgreement_A_SIDHp434_Compressed(secret_key_A.data(), public_key_B.data(),
                                                 shared_secret.data());
}

void SIDHp434_compressed::agree_B(std::span<uint8_t> shared_secret,
                                  std::span<const uint8_t> secret_key_B,
                                  std::span<const uint8_t> public_key_A) {
  if (shared_secret.size() != SharedSecretLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect shared secret size (actual: {}, expected: {})",
                                         shared_secret.size(), SharedSecretLength));
  }
  if (secret_key_B.size() != SecretKeyBLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect secret key B size (actual: {}, expected: {})",
                                         secret_key_B.size(), SecretKeyBLength));
  }
  if (public_key_A.size() != PublicKeyLength) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect public key A size (actual: {}, expected: {})",
                                         public_key_A.size(), PublicKeyLength));
  }

  EphemeralSecretAgreement_B_SIDHp434_Compressed(secret_key_B.data(), public_key_A.data(),
                                                 shared_secret.data());
}

}  // namespace crypto
