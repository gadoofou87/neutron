#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace protocol {

namespace detail {

class ConnectionPrivate;

class StreamManager;

class StreamPrivate;

}  // namespace detail

class Stream {
 public:
  enum class ReliabilityType { Reliable, Rexmit, Timed };
  using ReliabilityValue = uint32_t;

 public:
  template <typename... Args>
  explicit Stream(Args&&... args);
  ~Stream();

  [[nodiscard]] bool is_readable() const;

  [[nodiscard]] size_t max_message_size() const;

  std::optional<std::vector<uint8_t>> read();

  [[nodiscard]] ReliabilityType rel_type() const;

  [[nodiscard]] ReliabilityValue rel_val() const;

  void set_reliability_params(bool unordered, ReliabilityType rel_type, ReliabilityValue rel_val);

  [[nodiscard]] bool unordered() const;

  void write(std::span<const uint8_t> message);

 private:
  const std::unique_ptr<detail::StreamPrivate> impl_;

 private:
  friend detail::StreamManager;
};

}  // namespace protocol
