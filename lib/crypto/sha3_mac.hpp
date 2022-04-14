#pragma once

#include <cstring>
#include <vector>

#include "helpers.hpp"
#include "sha3.hpp"

namespace crypto {

template <Sha3Concept Hasher>
struct SHA3_MAC {
  static void compute(std::span<uint8_t> output, std::span<const uint8_t> secret,
                      std::span<const uint8_t> message) {
    std::vector<uint8_t> input(secret.size() + message.size());
    std::memcpy(input.data() + 0, secret.data(), secret.size());
    std::memcpy(input.data() + secret.size(), message.data(), message.size());
    Hasher::hash(output, input);
    Helpers::memzero(input.data(), secret.size());
  }
};

}  // namespace crypto
