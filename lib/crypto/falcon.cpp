#include "falcon.hpp"

#include <fmt/core.h>

#include "helpers.hpp"
#include "utils/debug/assert.hpp"

extern "C" {
#include <falcon.h>
}

namespace crypto {

namespace {

constexpr size_t logn512 = 9;

void init_shake256_context(shake256_context *rng) {
  std::array<uint8_t, 48> seed;
  Helpers::randombytes_buf(seed.data(), seed.size());
  shake256_init_prng_from_seed(rng, seed.data(), seed.size());
}

}  // namespace

void Falcon512::generate_keypair(std::span<uint8_t> public_key, std::span<uint8_t> secret_key) {
  if (public_key.size() != PublicKeyLength) {
    throw std::runtime_error(fmt::format("Incorrect public key size (actual: {}, expected: {})",
                                         public_key.size(), PublicKeyLength));
  }
  if (secret_key.size() != SecretKeyLength) {
    throw std::runtime_error(fmt::format("Incorrect secret key size (actual: {}, expected: {})",
                                         secret_key.size(), SecretKeyLength));
  }

  shake256_context rng;
  init_shake256_context(&rng);

  std::array<uint8_t, FALCON_TMPSIZE_KEYGEN(logn512)> tmp;

  auto ret = falcon_keygen_make(&rng, logn512, secret_key.data(), secret_key.size(),
                                public_key.data(), public_key.size(), tmp.data(), tmp.size());

  if (ret != 0) {
    throw std::runtime_error(fmt::format("falcon_keygen_make() returned {}", ret));
  }
}

void Falcon512::sign(std::span<uint8_t> signature, std::span<const uint8_t> message,
                     std::span<const uint8_t> secret_key) {
  if (signature.size() != SignatureLength) {
    throw std::runtime_error(fmt::format("Incorrect signature size (actual: {}, expected: {})",
                                         signature.size(), SignatureLength));
  }
  if (secret_key.size() != SecretKeyLength) {
    throw std::runtime_error(fmt::format("Incorrect secret key size (actual: {}, expected: {})",
                                         secret_key.size(), SecretKeyLength));
  }

  shake256_context rng;
  init_shake256_context(&rng);

  std::array<uint8_t, FALCON_TMPSIZE_KEYGEN(logn512)> tmp;

  size_t sig_len = signature.size();

  auto ret =
      falcon_sign_dyn(&rng, signature.data(), &sig_len, FALCON_SIG_PADDED, secret_key.data(),
                      secret_key.size(), message.data(), message.size(), tmp.data(), tmp.size());

  ASSERT(sig_len == signature.size());

  if (ret != 0) {
    throw std::runtime_error(fmt::format("falcon_sign_dyn() returned {}", ret));
  }
}

bool Falcon512::verify(std::span<const uint8_t> signature, std::span<const uint8_t> message,
                       std::span<const uint8_t> public_key) {
  if (signature.size() != SignatureLength) {
    throw std::runtime_error(fmt::format("Incorrect signature size (actual: {}, expected: {})",
                                         signature.size(), SignatureLength));
  }
  if (public_key.size() != PublicKeyLength) {
    throw std::runtime_error(fmt::format("Incorrect public key size (actual: {}, expected: {})",
                                         public_key.size(), PublicKeyLength));
  }

  std::array<uint8_t, FALCON_TMPSIZE_KEYGEN(logn512)> tmp;

  auto ret =
      falcon_verify(signature.data(), signature.size(), FALCON_SIG_PADDED, public_key.data(),
                    public_key.size(), message.data(), message.size(), tmp.data(), tmp.size());

  if (ret != 0) {
    if (ret == FALCON_ERR_FORMAT || ret == FALCON_ERR_BADSIG) {
      return false;
    }

    throw std::runtime_error(fmt::format("falcon_verify() returned {}", ret));
  }

  return true;
}

}  // namespace crypto
