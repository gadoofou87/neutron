#include "argon2.hpp"

#include <argon2.h>

#include <string>
#include <vector>

#include "helpers.hpp"

namespace crypto {

namespace {

argon2_type to_argon2_type(Argon2::Type type) {
  switch (type) {
    case Argon2::Type::d:
      return argon2_type::Argon2_d;
    case Argon2::Type::i:
      return argon2_type::Argon2_i;
    case Argon2::Type::id:
      return argon2_type::Argon2_id;
  }
}

}  // namespace

std::string Argon2::hash(std::string_view password, const Configuration &config) {
  auto type = to_argon2_type(config.type);

  std::vector<uint8_t> salt(config.saltlen);

  Helpers::randombytes_buf(salt.data(), salt.size());

  std::string encoded;

  encoded.resize(argon2_encodedlen(config.t_cost, config.m_cost, config.parallelism, config.saltlen,
                                   config.hashlen, type));

  auto ret = argon2_hash(config.t_cost, config.m_cost, config.parallelism, password.data(),
                         password.size(), salt.data(), salt.size(), nullptr, 0, encoded.data(),
                         encoded.size(), type, ARGON2_VERSION_NUMBER);

  if (ret != ARGON2_OK) {
    throw std::runtime_error(argon2_error_message(ret));
  }

  return encoded;
}

bool Argon2::verify(std::string_view encoded, std::string_view password, Type type) {
  auto ret = argon2_verify(encoded.data(), password.data(), password.size(), to_argon2_type(type));

  if (ret != ARGON2_OK) {
    if (ret == ARGON2_VERIFY_MISMATCH) {
      return false;
    } else {
      throw std::runtime_error(argon2_error_message(ret));
    }
  }

  return true;
}

}  // namespace crypto
