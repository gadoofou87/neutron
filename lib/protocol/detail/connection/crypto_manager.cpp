#include "crypto_manager.hpp"

#include <cstring>

#include "crypto/helpers.hpp"

namespace protocol {

namespace detail {

namespace {

auto generate_iv(const serialization::PackedInteger<Nonce>& nonce,
                 const serialization::pu32& initial_count) {
  thread_local std::array<uint8_t, crypto::ChaCha20Poly1305::IVSize> iv;

  ASSERT(nonce.size() + initial_count.size() == iv.size());

  std::memcpy(iv.data() + 0, nonce.data(), nonce.size());
  std::memcpy(iv.data() + nonce.size(), initial_count.data(), initial_count.size());

  return iv;
}

}  // namespace

bool CryptoManager::decrypt(std::span<const uint8_t> mac,
                            const serialization::PackedInteger<Nonce>& nonce,
                            std::span<uint8_t> data) {
  if (!replay_.check(nonce)) {
    return false;
  }

  ASSERT(decrypt_initial_count_ != encrypt_initial_count_);

  if (!crypto::ChaCha20Poly1305::decrypt(data, mac, generate_iv(nonce, decrypt_initial_count_),
                                         key_, {}, data)) {
    return false;
  }

  replay_.update(nonce);

  return true;
}

void CryptoManager::encrypt(std::span<uint8_t> mac, serialization::PackedInteger<Nonce>& nonce,
                            std::span<uint8_t> data) {
  nonce = ++nonce_;

  ASSERT(encrypt_initial_count_ != decrypt_initial_count_);

  crypto::ChaCha20Poly1305::encrypt(data, mac, generate_iv(nonce, encrypt_initial_count_), key_, {},
                                    data);
}

std::span<uint8_t> CryptoManager::key_buffer() {
  crypto::Helpers::memzero(key_.data(), key_.size());
  return key_;
}

void CryptoManager::reset() {
  decrypt_initial_count_ = -1;
  encrypt_initial_count_ = -1;
  crypto::Helpers::memzero(key_.data(), key_.size());
  nonce_ = 0;
  replay_.reset();
}

void CryptoManager::set_decrypt_initial_count(const serialization::pu32& initial_count) {
  decrypt_initial_count_ = initial_count;
}

void CryptoManager::set_encrypt_initial_count(const serialization::pu32& initial_count) {
  encrypt_initial_count_ = initial_count;
}

}  // namespace detail

}  // namespace protocol
