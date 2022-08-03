#include "sha3.hpp"

#include <fmt/core.h>

extern "C" {
#include <fips202.h>
}

namespace crypto {

void SHAKE128::hash(std::span<uint8_t> output, std::span<const uint8_t> input) {
  shake128(output.data(), output.size(), input.data(), input.size());
}

void SHAKE256::hash(std::span<uint8_t> output, std::span<const uint8_t> input) {
  shake256(output.data(), output.size(), input.data(), input.size());
}

void SHA3_256::hash(std::span<uint8_t> output, std::span<const uint8_t> input) {
  if (output.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect output size (actual: {}, expected: {})", output.size(), DigestSize));
  }

  sha3_256(output.data(), input.data(), input.size());
}

void SHA3_384::hash(std::span<uint8_t> output, std::span<const uint8_t> input) {
  if (output.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect output size (actual: {}, expected: {})", output.size(), DigestSize));
  }

  sha3_384(output.data(), input.data(), input.size());
}

void SHA3_512::hash(std::span<uint8_t> output, std::span<const uint8_t> input) {
  if (output.size() != DigestSize) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("Incorrect output size (actual: {}, expected: {})", output.size(), DigestSize));
  }

  sha3_512(output.data(), input.data(), input.size());
}

}  // namespace crypto
