#pragma once

#include <cstdint>
#include <string_view>

namespace crypto {

struct Argon2 {
  enum class Type { d, i, id };

  struct Configuration {
    uint32_t t_cost;
    uint32_t m_cost;
    uint32_t parallelism;
    size_t saltlen;
    size_t hashlen;
    Type type;
  };

  static std::string hash(std::string_view password, const Configuration& config);

  [[nodiscard]] static bool verify(std::string_view encoded, std::string_view password, Type type);
};

}  // namespace crypto
