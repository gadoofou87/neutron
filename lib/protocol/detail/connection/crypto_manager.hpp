#pragma once

#include "anti_replay_window.hpp"
#include "api/types/nonce.hpp"
#include "crypto/chacha20poly1305.hpp"
#include "serialization/packed_integer.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class CryptoManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  bool decrypt(std::span<const uint8_t> mac, const serialization::PackedInteger<Nonce>& nonce,
               std::span<uint8_t> data);

  void encrypt(std::span<uint8_t> mac, serialization::PackedInteger<Nonce>& nonce,
               std::span<uint8_t> data);

  std::span<uint8_t> key_buffer();

  void reset() override;

  void set_decrypt_initial_count(const serialization::pu32& initial_count);

  void set_encrypt_initial_count(const serialization::pu32& initial_count);

 private:
  serialization::pu32 decrypt_initial_count_;
  serialization::pu32 encrypt_initial_count_;
  std::array<uint8_t, crypto::ChaCha20Poly1305::KeyLength> key_;
  Nonce nonce_;
  AntiReplayWindow<Nonce> replay_;
};

}  // namespace detail

}  // namespace protocol
