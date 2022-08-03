#include "chacha20poly1305.hpp"

#include <fmt/core.h>

#include "helpers.hpp"

extern "C" {
#include <ecrypt-sync.h>
#include <poly1305-donna.h>
}

namespace crypto {

namespace {

constexpr std::array<uint8_t, 128> zero = {0};

struct chacha20poly1305_ctx {
  ECRYPT_ctx chacha20;
  poly1305_context poly1305;
};

void poly1305_pad_init(chacha20poly1305_ctx *ctx, const uint8_t *ad, size_t ad_size) {
  std::array<uint8_t, 64> block0 = {0};

  ECRYPT_encrypt_bytes(&ctx->chacha20, block0.data(), block0.data(), block0.size());
  poly1305_init(&ctx->poly1305, block0.data());

  poly1305_update(&ctx->poly1305, ad, ad_size);
  poly1305_update(&ctx->poly1305, zero.data(), 16 - ad_size % 16);

  Helpers::memzero(block0.data(), block0.size());
}

void xchacha20poly1305_init(chacha20poly1305_ctx *ctx, const uint8_t *key, const uint8_t *nonce,
                            const uint8_t *ad, size_t ad_size) {
  std::array<uint8_t, 32> subkey;
  ECRYPT_ctx tmp;

  ECRYPT_keysetup(&tmp, key, 256);
  tmp.input[12] = U8TO32_LITTLE(nonce + 0);
  tmp.input[13] = U8TO32_LITTLE(nonce + 4);
  tmp.input[14] = U8TO32_LITTLE(nonce + 8);
  tmp.input[15] = U8TO32_LITTLE(nonce + 12);
  hchacha20(&tmp, subkey.data());

  ECRYPT_keysetup(&ctx->chacha20, subkey.data(), 256);
  ECRYPT_ivsetup(&ctx->chacha20, nonce + 16);

  poly1305_pad_init(ctx, ad, ad_size);

  Helpers::memzero(subkey.data(), subkey.size());
  Helpers::memzero(&tmp, sizeof(tmp));
}

void chacha20poly1305_init(chacha20poly1305_ctx *ctx, const uint8_t *key, const uint8_t *nonce,
                           const uint8_t *ad, size_t ad_size) {
  ECRYPT_keysetup(&ctx->chacha20, key, 256);
  ctx->chacha20.input[12] = 0;
  ctx->chacha20.input[13] = U8TO32_LITTLE(nonce + 0);
  ctx->chacha20.input[14] = U8TO32_LITTLE(nonce + 4);
  ctx->chacha20.input[15] = U8TO32_LITTLE(nonce + 8);

  poly1305_pad_init(ctx, ad, ad_size);
}

void chacha20poly1305_encrypt(chacha20poly1305_ctx *ctx, const uint8_t *m, uint8_t *c,
                              size_t bytes) {
  ECRYPT_encrypt_bytes(&ctx->chacha20, m, c, bytes);
  poly1305_update(&ctx->poly1305, c, bytes);
}

void chacha20poly1305_decrypt(chacha20poly1305_ctx *ctx, const uint8_t *c, uint8_t *m,
                              size_t bytes) {
  poly1305_update(&ctx->poly1305, c, bytes);
  ECRYPT_decrypt_bytes(&ctx->chacha20, c, m, bytes);
}

void chacha20poly1305_finish(chacha20poly1305_ctx *ctx, uint8_t *mac, uint64_t ad_size,
                             uint64_t ct_size) {
  poly1305_update(&ctx->poly1305, zero.data(), 16 - ct_size % 16);

  std::array<uint8_t, sizeof(uint64_t) * 2> lengths;
  U64TO8_LITTLE(lengths.data() + sizeof(uint64_t) * 0, ad_size);
  U64TO8_LITTLE(lengths.data() + sizeof(uint64_t) * 1, ct_size);

  poly1305_update(&ctx->poly1305, lengths.data(), lengths.size());
  poly1305_finish(&ctx->poly1305, mac);

  Helpers::memzero(&ctx->chacha20, sizeof(ctx->chacha20));
  Helpers::memzero(&ctx->poly1305, sizeof(ctx->poly1305));
}

}  // namespace

void ChaCha20Poly1305::encrypt(std::span<uint8_t> ciphertext, std::span<uint8_t> mac,
                               std::span<const uint8_t> iv, std::span<const uint8_t> key,
                               std::span<const uint8_t> ad, std::span<const uint8_t> message) {
  if (message.size() != ciphertext.size()) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect message size (actual: {}, expected: {})",
                                         message.size(), ciphertext.size()));
  }
  if (mac.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect mac size (actual: {}, expected: {})", mac.size(), DigestSize));
  }
  if (iv.size() != IVSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect iv size (actual: {}, expected: {})", iv.size(), IVSize));
  }
  if (key.size() != KeyLength) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect key size (actual: {}, expected: {})", key.size(), KeyLength));
  }

  chacha20poly1305_ctx ctx;
  chacha20poly1305_init(&ctx, key.data(), iv.data(), ad.data(), ad.size());
  chacha20poly1305_encrypt(&ctx, message.data(), ciphertext.data(), message.size());
  chacha20poly1305_finish(&ctx, mac.data(), ad.size(), message.size());
}

bool ChaCha20Poly1305::decrypt(std::span<uint8_t> message, std::span<const uint8_t> mac,
                               std::span<const uint8_t> iv, std::span<const uint8_t> key,
                               std::span<const uint8_t> ad, std::span<const uint8_t> ciphertext) {
  if (message.size() != ciphertext.size()) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect message size (actual: {}, expected: {})",
                                         message.size(), ciphertext.size()));
  }
  if (mac.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect mac size (actual: {}, expected: {})", mac.size(), DigestSize));
  }
  if (iv.size() != IVSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect iv size (actual: {}, expected: {})", iv.size(), IVSize));
  }
  if (key.size() != KeyLength) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect key size (actual: {}, expected: {})", key.size(), KeyLength));
  }

  std::array<uint8_t, DigestSize> real_mac;

  chacha20poly1305_ctx ctx;
  chacha20poly1305_init(&ctx, key.data(), iv.data(), ad.data(), ad.size());
  chacha20poly1305_decrypt(&ctx, ciphertext.data(), message.data(), ciphertext.size());
  chacha20poly1305_finish(&ctx, real_mac.data(), ad.size(), ciphertext.size());

  return poly1305_verify(real_mac.data(), mac.data()) != 0;
}

void XChaCha20Poly1305::encrypt(std::span<uint8_t> ciphertext, std::span<uint8_t> mac,
                                std::span<const uint8_t> iv, std::span<const uint8_t> key,
                                std::span<const uint8_t> ad, std::span<const uint8_t> message) {
  if (message.size() != ciphertext.size()) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect message size (actual: {}, expected: {})",
                                         message.size(), ciphertext.size()));
  }
  if (mac.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect mac size (actual: {}, expected: {})", mac.size(), DigestSize));
  }
  if (iv.size() != IVSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect iv size (actual: {}, expected: {})", iv.size(), IVSize));
  }
  if (key.size() != KeyLength) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect key size (actual: {}, expected: {})", key.size(), KeyLength));
  }

  chacha20poly1305_ctx ctx;
  xchacha20poly1305_init(&ctx, key.data(), iv.data(), ad.data(), ad.size());
  chacha20poly1305_encrypt(&ctx, message.data(), ciphertext.data(), message.size());
  chacha20poly1305_finish(&ctx, mac.data(), ad.size(), message.size());
}

bool XChaCha20Poly1305::decrypt(std::span<uint8_t> message, std::span<const uint8_t> mac,
                                std::span<const uint8_t> iv, std::span<const uint8_t> key,
                                std::span<const uint8_t> ad, std::span<const uint8_t> ciphertext) {
  if (message.size() != ciphertext.size()) [[unlikely]] {
    throw std::runtime_error(fmt::format("Incorrect message size (actual: {}, expected: {})",
                                         message.size(), ciphertext.size()));
  }
  if (mac.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect mac size (actual: {}, expected: {})", mac.size(), DigestSize));
  }
  if (iv.size() != IVSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect iv size (actual: {}, expected: {})", iv.size(), IVSize));
  }
  if (key.size() != KeyLength) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect key size (actual: {}, expected: {})", key.size(), KeyLength));
  }

  std::array<uint8_t, DigestSize> real_mac;

  chacha20poly1305_ctx ctx;
  xchacha20poly1305_init(&ctx, key.data(), iv.data(), ad.data(), ad.size());
  chacha20poly1305_decrypt(&ctx, ciphertext.data(), message.data(), ciphertext.size());
  chacha20poly1305_finish(&ctx, real_mac.data(), ad.size(), ciphertext.size());

  return poly1305_verify(real_mac.data(), mac.data()) != 0;
}

}  // namespace crypto
