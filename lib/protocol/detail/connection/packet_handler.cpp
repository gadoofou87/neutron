#include "packet_handler.hpp"

#include "api/structures/abort.hpp"
#include "api/structures/chunk.hpp"
#include "api/structures/chunk_list.hpp"
#include "api/structures/encrypted_packet_data.hpp"
#include "api/structures/forward_cumulative_tsn.hpp"
#include "api/structures/heartbeat_acknowledgement.hpp"
#include "api/structures/heartbeat_request.hpp"
#include "api/structures/initiation.hpp"
#include "api/structures/initiation_acknowledgement.hpp"
#include "api/structures/initiation_complete.hpp"
#include "api/structures/packet.hpp"
#include "api/structures/payload_data.hpp"
#include "api/structures/selective_acknowledgement.hpp"
#include "api/structures/shutdown_acknowledgement.hpp"
#include "api/structures/shutdown_association.hpp"
#include "api/structures/shutdown_complete.hpp"
#include "connection_p.hpp"
#include "crypto/helpers.hpp"
#include "crypto/sha3.hpp"
#include "crypto/sha3_mac.hpp"
#include "crypto/sidhp434_compressed.hpp"
#include "stream_p.hpp"

namespace protocol {

namespace detail {

void PacketHandler::reset() {}

template <>
void PacketHandler::handle(Abort abort) {
  if (parent().state_manager.any_of(Connection::State::Listen)) [[unlikely]] {
    return;
  }

  if (!abort.validate()) [[unlikely]] {
    return;
  }

  parent().state_manager.set(Connection::State::Closed);
}

template <>
void PacketHandler::handle(Initiation initiation) {
  if (parent().internal_data.type == Connection::Type::Client) [[unlikely]] {
    return;
  }
  if (parent().state_manager.none_of(Connection::State::Listen, Connection::State::InitReceived))
      [[unlikely]] {
    return;
  }

  if (!initiation.validate()) [[unlikely]] {
    return;
  }
  if (initiation.public_key_a().size() != crypto::SIDHp434_compressed::PublicKeyLength ||
      initiation.public_key_b().size() != crypto::SIDHp434_compressed::PublicKeyLength ||
      initiation.public_key_b_mac().size() != crypto::SHA3_256::DigestSize) [[unlikely]] {
    //
    return;
  }

  if (!parent().internal_data.stored_init_ack.has_value()) {
    crypto::Helpers::memzero(parent().internal_data.temp_agreed.data(),
                             parent().internal_data.temp_agreed.size());

    crypto::SIDHp434_compressed::agree_B(parent().internal_data.temp_agreed,
                                         parent().internal_data.secret_key_b,
                                         initiation.public_key_a());

    crypto::Helpers::memzero(parent().internal_data.secret_key_b.data(),
                             parent().internal_data.secret_key_b.size());

    std::array<uint8_t, crypto::SHA3_256::DigestSize> public_key_b_mac;

    crypto::SHA3_MAC<crypto::SHA3_256>::compute(
        public_key_b_mac, parent().internal_data.temp_agreed, initiation.public_key_b());

    if (!crypto::Helpers::memcmp(public_key_b_mac.data(), initiation.public_key_b_mac().data(),
                                 public_key_b_mac.size())) [[unlikely]] {
      //
      return;
    }

    static constexpr size_t public_key_a_size = crypto::SIDHp434_compressed::PublicKeyLength;
    static constexpr size_t public_key_a_mac_size = crypto::SHA3_256::DigestSize;

    auto buffer = serialization::BufferBuilder<InitiationAcknowledgement>{}
                      .set_public_key_a_size(public_key_a_size)
                      .set_public_key_a_mac_size(public_key_a_mac_size)
                      .build();

    InitiationAcknowledgement init_ack(buffer);

    init_ack.connection_id() = parent().internal_data.connection_id;
    init_ack.public_key_a().size() = public_key_a_size;
    init_ack.public_key_a_mac().size() = public_key_a_mac_size;

    std::array<uint8_t, crypto::SIDHp434_compressed::SecretKeyALength> secret_key_a;

    crypto::SIDHp434_compressed::generate_keypair_A(init_ack.public_key_a(), secret_key_a);

    crypto::SHA3_MAC<crypto::SHA3_256>::compute(
        init_ack.public_key_a_mac(), parent().internal_data.temp_agreed, init_ack.public_key_a());

    crypto::Helpers::memzero(parent().internal_data.temp_agreed.data(),
                             parent().internal_data.temp_agreed.size());

    std::array<uint8_t, crypto::SIDHp434_compressed::SharedSecretLength> agreed;

    crypto::SIDHp434_compressed::agree_A(agreed, secret_key_a, initiation.public_key_b());

    crypto::Helpers::memzero(secret_key_a.data(), secret_key_a.size());

    crypto::SHAKE256::hash(parent().crypto_manager.key_buffer(), agreed);

    crypto::Helpers::memzero(agreed.data(), agreed.size());

    parent().internal_data.stored_init_ack = std::move(buffer);

    parent().state_manager.set(Connection::State::InitReceived);
  }

  parent().out_control_queue.push(ChunkType::InitiationAcknowledgement,
                                  *parent().internal_data.stored_init_ack);
}

template <>
void PacketHandler::handle(InitiationAcknowledgement initiation_ack) {
  if (parent().internal_data.type == Connection::Type::Server) [[unlikely]] {
    return;
  }
  if (parent().state_manager.none_of(Connection::State::InitSent)) [[unlikely]] {
    return;
  }

  if (!initiation_ack.validate()) [[unlikely]] {
    return;
  }
  if (initiation_ack.public_key_a().size() != crypto::SIDHp434_compressed::PublicKeyLength ||
      initiation_ack.public_key_a_mac().size() != crypto::SHA3_256::DigestSize) [[unlikely]] {
    return;
  }

  parent().internal_data.connection_id = initiation_ack.connection_id();

  std::array<uint8_t, crypto::SHA3_256::DigestSize> public_key_a_mac;

  crypto::SHA3_MAC<crypto::SHA3_256>::compute(public_key_a_mac, parent().internal_data.temp_agreed,
                                              initiation_ack.public_key_a());

  crypto::Helpers::memzero(parent().internal_data.temp_agreed.data(),
                           parent().internal_data.temp_agreed.size());

  if (!crypto::Helpers::memcmp(public_key_a_mac.data(), initiation_ack.public_key_a_mac().data(),
                               public_key_a_mac.size())) [[unlikely]] {
    //
    return;
  }

  std::array<uint8_t, crypto::SIDHp434_compressed::SharedSecretLength> agreed;

  crypto::SIDHp434_compressed::agree_B(agreed, parent().internal_data.secret_key_b,
                                       initiation_ack.public_key_a());

  crypto::Helpers::memzero(parent().internal_data.secret_key_b.data(),
                           parent().internal_data.secret_key_b.size());

  crypto::SHAKE256::hash(parent().crypto_manager.key_buffer(), agreed);

  crypto::Helpers::memzero(agreed.data(), agreed.size());

  parent().timer_manager.stop<TimerManager::TimerId::Init>();

  parent().state_manager.set(Connection::State::Established);

  parent().out_control_queue.push(ChunkType::InitiationComplete, {});
}

template <>
void PacketHandler::handle(InitiationComplete initiation_complete) {
  if (parent().internal_data.type == Connection::Type::Client) [[unlikely]] {
    return;
  }
  if (parent().state_manager.none_of(Connection::State::InitReceived)) [[unlikely]] {
    return;
  }

  if (!initiation_complete.validate()) [[unlikely]] {
    return;
  }

  parent().state_manager.set(Connection::State::Established);
}

template <>
void PacketHandler::handle(PayloadData payload_data) {
  if (parent().state_manager.none_of(
          Connection::State::InitReceived, Connection::State::Established,
          Connection::State::ShutdownPending, Connection::State::ShutdownSent)) [[unlikely]] {
    return;
  }

  if (!payload_data.validate()) [[unlikely]] {
    return;
  }

  auto ret_val = parent().in_data_queue.push(payload_data);

  if (!ret_val.success || ret_val.has_packet_loss) {
    parent().ack_manager.trigger_immediate_ack();
  }
  if (!ret_val.success) {
    return;
  }
  if (ret_val.user_data.has_value()) {
    auto& stream_private = parent().stream_manager.get_private(payload_data.sid());

    stream_private.handle_data(payload_data.bits().u, payload_data.ssn(),
                               std::move(*ret_val.user_data));
  }

  parent().ack_manager.trigger_delayed_ack();
}

template <>
void PacketHandler::handle(SelectiveAcknowledgement sack) {
  if (parent().state_manager.none_of(Connection::State::Established,
                                     Connection::State::ShutdownPending,
                                     Connection::State::ShutdownReceived)) [[unlikely]] {
    return;
  }

  if (!sack.validate()) [[unlikely]] {
    return;
  }

  if (TransmissionSequenceNumber::Greater{}(parent().out_data_queue.cum_tsn_ack_point(),
                                            sack.cum_tsn_ack())) {
    return;
  }

  const bool cum_tsn_ack_point_advanced = TransmissionSequenceNumber::Less{}(
      parent().out_data_queue.cum_tsn_ack_point(), sack.cum_tsn_ack());

  size_t bytes_acked = 0;

  if (cum_tsn_ack_point_advanced) {
    parent().timer_manager.stop<TimerManager::TimerId::Rtx>();

    bytes_acked += parent().out_data_queue.acknowledge(sack.cum_tsn_ack());

    if (parent().congestion_manager.in_fast_recovery() &&
        TransmissionSequenceNumber::Greater{}(
            sack.cum_tsn_ack(), parent().congestion_manager.fast_recover_exit_point())) {
      parent().congestion_manager.exit_fast_recovery();
    }
  }

  TransmissionSequenceNumber::value_type htna;

  bytes_acked += parent().out_data_queue.acknowledge(htna, sack.gap_ack_blks());

  parent().congestion_manager.acknowledged(bytes_acked, cum_tsn_ack_point_advanced);

  if (parent().out_data_queue.empty()) {
    if (parent().state_manager.any_of(Connection::State::Established)) {
      parent().timer_manager.start<TimerManager::TimerId::Heartbeat>();
    } else if (parent().state_manager.any_of(Connection::State::ShutdownPending)) {
      parent().out_control_queue.push(ChunkType::ShutdownAssociation, {});

      parent().state_manager.set(Connection::State::ShutdownSent);
    } else if (parent().state_manager.any_of(Connection::State::ShutdownReceived)) {
      parent().out_control_queue.push(ChunkType::ShutdownAcknowledgement, {});

      parent().state_manager.set(Connection::State::ShutdownAckSent);
    }
    return;
  }

  if (!parent().congestion_manager.in_fast_recovery()) {
    parent().out_data_queue.inc_miss_indications(htna);
  } else if (cum_tsn_ack_point_advanced) {
    parent().out_data_queue.inc_miss_indications(parent().out_data_queue.my_next_tsn());
  }

  parent().out_data_queue.advance_advanced_peer_tsn_ack_point();

  if (cum_tsn_ack_point_advanced) {
    parent().timer_manager.start<TimerManager::TimerId::Rtx>();
  }
}

template <>
void PacketHandler::handle(HeartbeatRequest heartbeat_request) {
  if (!heartbeat_request.validate()) [[unlikely]] {
    return;
  }

  auto buffer = serialization::BufferBuilder<HeartbeatAcknowledgement>{}.build();

  HeartbeatAcknowledgement heartbeat_ack(buffer);

  heartbeat_ack.hb_info() = heartbeat_request.hb_info();

  parent().out_control_queue.push(ChunkType::HeartbeatAcknowledgement, std::move(buffer));
}

template <>
void PacketHandler::handle(HeartbeatAcknowledgement heartbeat_ack) {
  if (!heartbeat_ack.validate()) [[unlikely]] {
    return;
  }

  //

  const auto rtt = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now().time_since_epoch())
                       .count() -
                   heartbeat_ack.hb_info().time_value;

  if (rtt < 0) [[unlikely]] {
    return;
  }

  parent().rto_manager.recalculate(rtt);
}

template <>
void PacketHandler::handle(ShutdownAssociation shutdown_association) {
  if (parent().state_manager.none_of(Connection::State::Established)) [[unlikely]] {
    return;
  }

  if (!shutdown_association.validate()) [[unlikely]] {
    return;
  }

  if (parent().out_data_queue.empty()) {
    parent().out_control_queue.push(ChunkType::ShutdownAcknowledgement, {});

    parent().state_manager.set(Connection::State::ShutdownAckSent);
  } else {
    parent().state_manager.set(Connection::State::ShutdownReceived);
  }
}

template <>
void PacketHandler::handle(ShutdownAcknowledgement shutdown_acknowledgement) {
  if (parent().state_manager.none_of(Connection::State::ShutdownSent)) [[unlikely]] {
    return;
  }

  if (!shutdown_acknowledgement.validate()) [[unlikely]] {
    return;
  }

  parent().out_control_queue.push(ChunkType::ShutdownComplete, {});

  parent().state_manager.set(Connection::State::Closed);
}

template <>
void PacketHandler::handle(ShutdownComplete shutdown_complete) {
  if (parent().state_manager.none_of(Connection::State::ShutdownAckSent)) [[unlikely]] {
    return;
  }

  if (!shutdown_complete.validate()) [[unlikely]] {
    return;
  }

  parent().state_manager.set(Connection::State::Closed);
}

template <>
void PacketHandler::handle(ForwardCumulativeTSN forward_cumulative_tsn) {
  if (!forward_cumulative_tsn.validate()) [[unlikely]] {
    return;
  }

  //
}

template <>
void PacketHandler::handle(Chunk chunk) {
  if (!chunk.validate()) [[unlikely]] {
    return;
  }

  switch (chunk.type()) {
    case ChunkType::Abort:
      handle(Abort(chunk.data()));
      break;
    case ChunkType::Initiation:
      handle(Initiation(chunk.data()));
      break;
    case ChunkType::InitiationAcknowledgement:
      handle(InitiationAcknowledgement(chunk.data()));
      break;
    case ChunkType::InitiationComplete:
      handle(InitiationComplete(chunk.data()));
      break;
    case ChunkType::PayloadData:
      handle(PayloadData(chunk.data()));
      break;
    case ChunkType::SelectiveAcknowledgement:
      handle(SelectiveAcknowledgement(chunk.data()));
      break;
    case ChunkType::HeartbeatRequest:
      handle(HeartbeatRequest(chunk.data()));
      break;
    case ChunkType::HeartbeatAcknowledgement:
      handle(HeartbeatAcknowledgement(chunk.data()));
      break;
    case ChunkType::ShutdownAssociation:
      handle(ShutdownAssociation(chunk.data()));
      break;
    case ChunkType::ShutdownAcknowledgement:
      handle(ShutdownAcknowledgement(chunk.data()));
      break;
    case ChunkType::ShutdownComplete:
      handle(ShutdownComplete(chunk.data()));
      break;
    case ChunkType::ForwardCumulativeTSN:
      handle(ForwardCumulativeTSN(chunk.data()));
      break;
  }
}

template <>
void PacketHandler::handle(ChunkList chunk_list) {
  if (!chunk_list.validate()) [[unlikely]] {
    return;
  }

  for (size_t i = 0; i != chunk_list.size(); ++i) {
    handle(Chunk(chunk_list.chunk_data(i)));
  }

  parent().ack_manager.commit();

  parent().network_manager.write_pending_packets();
}

bool PacketHandler::handle(Packet packet) {
  if (!packet.validate()) [[unlikely]] {
    return false;
  }
  if (parent().state_manager.none_of(Connection::State::Listen, Connection::State::InitSent))
      [[unlikely]] {
    if (packet.connection_id() != parent().internal_data.connection_id) {
      return false;
    }
  }

  std::span<uint8_t> data;

  if (packet.bits().e) [[likely]] {
    EncryptedPacketData encrypted_packet_data(packet.data());

    if (!encrypted_packet_data.validate()) [[unlikely]] {
      return false;
    }

    if (!parent().crypto_manager.decrypt(encrypted_packet_data.mac(), encrypted_packet_data.nonce(),
                                         encrypted_packet_data.data())) [[unlikely]] {
      return false;
    }

    data = encrypted_packet_data.data();
  } else {
    data = packet.data();
  }

  handle(ChunkList(data));

  return true;
}

}  // namespace detail

}  // namespace protocol
