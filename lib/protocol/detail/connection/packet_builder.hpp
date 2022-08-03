#pragma once

#include <cstdint>
#include <list>
#include <span>
#include <vector>

#include "api/structures/chunk.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class PacketBuilder : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using BuildInput =
      std::vector<std::pair<ChunkType, std::reference_wrapper<const std::vector<uint8_t>>>>;
  using BuildOutput = std::list<std::vector<uint8_t>>;

  using Mtu = uint16_t;

 public:
  explicit PacketBuilder(parent_type& parent);

  BuildOutput build(BuildInput&& input);

  [[nodiscard]] size_t max_chunk_data_size(ChunkType type) const;

  [[nodiscard]] Mtu mtu() const;

  void reset() override;

  void set_mtu(Mtu mtu);

 private:
  template <bool Encrypted>
  BuildOutput build(std::list<std::vector<uint8_t>>&& chunks);

  std::vector<uint8_t> build_chunk_list(std::list<std::vector<uint8_t>>& chunks,
                                        size_t cached_max_chunk_list_size);

  std::vector<uint8_t> build_encrypted_packet_data(std::vector<uint8_t>&& packet_data);

  std::vector<uint8_t> build_packet(std::vector<uint8_t>&& packet_data, bool encrypted);

  [[nodiscard]] size_t max_chunk_list_chunk_data_size(bool encrypted) const;

  [[nodiscard]] size_t max_encrypted_packet_data_data_size() const;

  [[nodiscard]] size_t max_packet_data_size() const;

 private:
  Mtu mtu_;
};

}  // namespace detail

}  // namespace protocol
