#pragma once

#include <array>
#include <optional>

#include "connection.hpp"
#include "crypto/sidhp434_compressed.hpp"
#include "detail/connection/api/types/connection_id.hpp"

namespace protocol {

namespace detail {

struct InternalData {
  ConnectionID connection_id;
  std::array<uint8_t, crypto::SIDHp434_compressed::SecretKeyBLength> secret_key_b;
  std::optional<std::vector<uint8_t>> stored_init;
  std::optional<std::vector<uint8_t>> stored_init_ack;
  std::array<uint8_t, crypto::SIDHp434_compressed::SharedSecretLength> temp_agreed;
  Connection::Type type;
};

}  // namespace detail

}  // namespace protocol
