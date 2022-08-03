#include "packet_builder.hpp"

#include "api/structures/chunk.hpp"
#include "api/structures/chunk_list.hpp"
#include "api/structures/encrypted_packet_data.hpp"
#include "api/structures/packet.hpp"
#include "api/structures/payload_data.hpp"
#include "api/structures/selective_acknowledgement.hpp"
#include "connection_p.hpp"
#include "utils/debug/assert.hpp"
#include "utils/span/copy.hpp"

namespace protocol {

namespace detail {

namespace {

constexpr PacketBuilder::Mtu MTU_DEFAULT = 1228;

bool is_encryptable(ChunkType type) {
  if (type == ChunkType::Initiation) {
    return false;
  }
  if (type == ChunkType::InitiationAcknowledgement) {
    return false;
  }
  return true;
}

}  // namespace

PacketBuilder::PacketBuilder(parent_type& parent)
    : utils::Parentable<parent_type>(parent), mtu_(MTU_DEFAULT) {}

template <bool Encrypted>
PacketBuilder::BuildOutput PacketBuilder::build(std::list<std::vector<uint8_t>>&& chunks) {
  BuildOutput result;

  const size_t max_chunk_list_size =
      Encrypted ? max_encrypted_packet_data_data_size() : max_packet_data_size();

  while (!chunks.empty()) {
    auto chunk_list_buffer = build_chunk_list(chunks, max_chunk_list_size);

    if constexpr (Encrypted) {
      auto encrypted_packet_data_buffer = build_encrypted_packet_data(std::move(chunk_list_buffer));

      result.push_back(build_packet(std::move(encrypted_packet_data_buffer), true));
    } else {
      result.push_back(build_packet(std::move(chunk_list_buffer), false));
    }
  }

  return result;
}

PacketBuilder::BuildOutput PacketBuilder::build(BuildInput&& input) {
  std::list<std::vector<uint8_t>> e;
  std::list<std::vector<uint8_t>> u;

  for (auto iterator = input.begin(); iterator != input.end(); ++iterator) {
    auto buffer =
        serialization::BufferBuilder<Chunk>{}.set_data_size(iterator->second.get().size()).build();

    Chunk chunk(buffer);

    chunk.type() = iterator->first;

    if (!iterator->second.get().empty()) {
      utils::span::copy<uint8_t>(chunk.data(), iterator->second.get());
    }

    ASSERT(chunk.validate());

    if (is_encryptable(iterator->first)) [[likely]] {
      e.insert(std::lower_bound(e.cbegin(), e.cend(), buffer), std::move(buffer));
    } else {
      u.insert(std::lower_bound(u.cbegin(), u.cend(), buffer), std::move(buffer));
    }
  }

  BuildOutput result;

  if (!u.empty()) {
    result.splice(result.cend(), build<false>(std::move(u)));
  }
  if (!e.empty()) {
    result.splice(result.cend(), build<true>(std::move(e)));
  }

  return result;
}

size_t PacketBuilder::max_chunk_data_size(ChunkType type) const {
  size_t min = serialization::BufferBuilder<Chunk>{}.set_data_size(0).buffer_size();

  const size_t cached_max_chunk_list_chunk_data_size =
      max_chunk_list_chunk_data_size(is_encryptable(type));

  ASSERT(cached_max_chunk_list_chunk_data_size > min);
  return cached_max_chunk_list_chunk_data_size - min;
}

PacketBuilder::Mtu PacketBuilder::mtu() const { return mtu_; }

void PacketBuilder::reset() {}

void PacketBuilder::set_mtu(Mtu mtu) { mtu_ = mtu; }

std::vector<uint8_t> PacketBuilder::build_chunk_list(std::list<std::vector<uint8_t>>& chunks,
                                                     size_t max_chunk_list_size) {
  serialization::BufferBuilder<ChunkList> buffer_builder;

  size_t size = 0;

  for (const auto& chunk : chunks) {
    auto new_buffer_builder = buffer_builder.add_chunk_data_size(chunk.size());

    const size_t cached_new_size = new_buffer_builder.buffer_size();

    if (cached_new_size <= max_chunk_list_size) {
      buffer_builder = new_buffer_builder;
      ++size;
    }
    if (cached_new_size >= max_chunk_list_size) {
      break;
    }
  }

  auto buffer = buffer_builder.build();

  ChunkList chunk_list(buffer);

  chunk_list.size() = size;

  size_t n = 0;

  for (auto iterator = chunks.begin(); size > 0; --size, iterator = chunks.erase(iterator)) {
    ASSERT(iterator != chunks.end());

    auto& chunk_data = chunk_list.chunk_data(n++);

    chunk_data.size() = iterator->size();

    utils::span::copy<uint8_t>(chunk_data, *iterator);
  }

  ASSERT(chunk_list.validate());

  return buffer;
}

std::vector<uint8_t> PacketBuilder::build_encrypted_packet_data(
    std::vector<uint8_t>&& packet_data) {
  auto buffer =
      serialization::BufferBuilder<EncryptedPacketData>{}.set_data_size(packet_data.size()).build();

  EncryptedPacketData encrypted_packet_data(buffer);

  utils::span::copy<uint8_t>(encrypted_packet_data.data(), packet_data);

  parent().crypto_manager.encrypt(encrypted_packet_data.mac(), encrypted_packet_data.nonce(),
                                  encrypted_packet_data.data());

  ASSERT(encrypted_packet_data.validate());

  return buffer;
}

std::vector<uint8_t> PacketBuilder::build_packet(std::vector<uint8_t>&& packet_data,
                                                 bool encrypted) {
  auto buffer = serialization::BufferBuilder<Packet>{}.set_data_size(packet_data.size()).build();

  Packet packet(buffer);

  packet.bits().e = encrypted;
  packet.connection_id() = parent().internal_data.connection_id;

  utils::span::copy<uint8_t>(packet.data(), packet_data);

  ASSERT(packet.validate());

  return buffer;
}

size_t PacketBuilder::max_chunk_list_chunk_data_size(bool encrypted) const {
  const size_t min = serialization::BufferBuilder<ChunkList>{}.add_chunk_data_size(0).buffer_size();

  if (encrypted) {
    const size_t cached_max_encrypted_packet_data_data_size = max_encrypted_packet_data_data_size();

    ASSERT(cached_max_encrypted_packet_data_data_size > min);
    return cached_max_encrypted_packet_data_data_size - min;
  } else {
    const size_t cached_max_packet_data_size = max_packet_data_size();

    ASSERT(cached_max_packet_data_size > min);
    return cached_max_packet_data_size - min;
  }
}

size_t PacketBuilder::max_encrypted_packet_data_data_size() const {
  const size_t min =
      serialization::BufferBuilder<EncryptedPacketData>{}.set_data_size(0).buffer_size();

  const size_t cached_max_packet_data_size = max_packet_data_size();

  ASSERT(cached_max_packet_data_size > min);
  return cached_max_packet_data_size - min;
}

size_t PacketBuilder::max_packet_data_size() const {
  size_t min = serialization::BufferBuilder<Packet>{}.set_data_size(0).buffer_size();

  ASSERT(mtu_ > min);
  return mtu_ - min;
}

}  // namespace detail

}  // namespace protocol
