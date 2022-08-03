#include <limits>

#include "connection_p.hpp"
#include "crypto/helpers.hpp"
#include "crypto/sha3_mac.hpp"
#include "detail/connection/api/structures/initiation.hpp"
#include "utils/span/copy.hpp"

namespace protocol {

using namespace detail;

namespace {

constexpr uint32_t CLIENT_INITIAL_COUNT = 0;
constexpr uint32_t SERVER_INITIAL_COUNT = 1;

}  // namespace

Connection::Connection(asio::io_context& io_context) : impl_(new ConnectionPrivate(io_context)) {}

Connection::~Connection() {
  std::unique_lock lock(impl_->mutex);

  shutdown();
}

void Connection::abort() {
  std::unique_lock lock(impl_->mutex);

  if (impl_->state_manager.any_of(Connection::State::Closed)) {
    return;
  }

  if (impl_->state_manager.none_of(Connection::State::Listen)) {
    impl_->out_control_queue.push(ChunkType::Abort, {});

    impl_->network_manager.write_pending_packets<false>();
  }

  impl_->state_manager.set(State::Closed);
}

void Connection::associate(ClientConfiguration&& config) {
  if (config.rx_socket == nullptr) {
    throw std::runtime_error("rx_socket is null");
  }
  if (config.tx_socket == nullptr) {
    throw std::runtime_error("tx_socket is null");
  }

  if (config.peer_public_key.size() != crypto::SIDHp434_compressed::PublicKeyLength) {
    throw std::runtime_error("peer_public_key has incorrect size");
  }

  std::unique_lock lock(impl_->mutex);

  if (impl_->state_manager.none_of(State::Closed)) {
    throw std::runtime_error("connection is not closed");
  }

  impl_->reset();

  impl_->internal_data.connection_id = 0;
  impl_->internal_data.type = Type::Client;

  impl_->crypto_manager.set_decrypt_initial_count(SERVER_INITIAL_COUNT);
  impl_->crypto_manager.set_encrypt_initial_count(CLIENT_INITIAL_COUNT);

  impl_->network_manager.set_rx_socket(std::move(config.rx_socket));
  impl_->network_manager.set_tx_endpoint(std::move(config.peer_endpoint));
  impl_->network_manager.set_tx_socket(std::move(config.tx_socket));

  impl_->network_manager.start_receive();

  static constexpr size_t public_key_a_size = crypto::SIDHp434_compressed::PublicKeyLength;
  static constexpr size_t public_key_b_size = crypto::SIDHp434_compressed::PublicKeyLength;
  static constexpr size_t public_key_b_mac_size = crypto::SHA3_256::DigestSize;

  auto buffer = serialization::BufferBuilder<Initiation>{}
                    .set_public_key_a_size(public_key_a_size)
                    .set_public_key_b_size(public_key_b_size)
                    .set_public_key_b_mac_size(public_key_b_mac_size)
                    .build();

  Initiation init(buffer);

  init.public_key_a().size() = public_key_a_size;
  init.public_key_b().size() = public_key_b_size;
  init.public_key_b_mac().size() = public_key_b_mac_size;

  std::array<uint8_t, crypto::SIDHp434_compressed::SecretKeyALength> secret_key_a;

  crypto::SIDHp434_compressed::generate_keypair_A(init.public_key_a(), secret_key_a);

  crypto::Helpers::memzero(impl_->internal_data.temp_agreed.data(),
                           impl_->internal_data.temp_agreed.size());

  crypto::SIDHp434_compressed::agree_A(impl_->internal_data.temp_agreed, secret_key_a,
                                       config.peer_public_key);

  crypto::Helpers::memzero(secret_key_a.data(), secret_key_a.size());

  crypto::Helpers::memzero(impl_->internal_data.secret_key_b.data(),
                           impl_->internal_data.secret_key_b.size());

  crypto::SIDHp434_compressed::generate_keypair_B(init.public_key_b(),
                                                  impl_->internal_data.secret_key_b);

  crypto::SHA3_MAC<crypto::SHA3_256>::compute(
      init.public_key_b_mac(), impl_->internal_data.temp_agreed, init.public_key_b());

  impl_->internal_data.stored_init = std::move(buffer);

  impl_->state_manager.set(State::InitSent);

  impl_->out_control_queue.push(ChunkType::Initiation, *impl_->internal_data.stored_init);

  impl_->network_manager.write_pending_packets();

  impl_->timer_manager.start<TimerManager::TimerId::Init>();
}

void Connection::associate(ServerConfiguration&& config) {
  if (config.rx_socket == nullptr) {
    throw std::runtime_error("rx_socket is null");
  }
  if (config.tx_socket == nullptr) {
    throw std::runtime_error("tx_socket is null");
  }

  if (config.secret_key.size() != crypto::SIDHp434_compressed::SecretKeyBLength) {
    throw std::runtime_error("secret_key has incorrect size");
  }

  std::unique_lock lock(impl_->mutex);

  if (impl_->state_manager.none_of(State::Closed)) {
    throw std::runtime_error("connection is not closed");
  }

  impl_->reset();

  impl_->internal_data.connection_id = config.connection_id;
  impl_->internal_data.type = Type::Server;

  impl_->crypto_manager.set_decrypt_initial_count(CLIENT_INITIAL_COUNT);
  impl_->crypto_manager.set_encrypt_initial_count(SERVER_INITIAL_COUNT);

  impl_->network_manager.set_rx_socket(std::move(config.rx_socket));
  impl_->network_manager.set_tx_endpoint(std::move(config.peer_endpoint));
  impl_->network_manager.set_tx_socket(std::move(config.tx_socket));

  impl_->network_manager.start_receive();

  crypto::Helpers::memzero(impl_->internal_data.secret_key_b.data(),
                           impl_->internal_data.secret_key_b.size());

  utils::span::copy<uint8_t>(impl_->internal_data.secret_key_b, config.secret_key);

  crypto::Helpers::memzero(config.secret_key.data(), config.secret_key.size());

  impl_->state_manager.set(State::Listen);
}

size_t Connection::max_num_streams() const { return std::numeric_limits<StreamIdentifier>::max(); }

//

std::optional<size_t> Connection::readable_stream() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->stream_manager.find_readable();
}

void Connection::shutdown() {
  std::unique_lock lock(impl_->mutex);

  if (impl_->state_manager.any_of(State::Listen)) {
    impl_->state_manager.set(State::Closed);
    return;
  }

  if (impl_->state_manager.none_of(State::Established)) {
    return;
  }

  if (impl_->out_data_queue.empty()) {
    impl_->out_control_queue.push(ChunkType::ShutdownAssociation, {});

    impl_->network_manager.write_pending_packets();

    impl_->state_manager.set(State::ShutdownSent);
  } else {
    impl_->state_manager.set(State::ShutdownPending);
  }
}

Connection::State Connection::state() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->state_manager.get();
}

Connection::Type Connection::type() const {
  std::unique_lock lock(impl_->mutex);

  return impl_->internal_data.type;
}

const Stream& Connection::operator[](std::size_t stream_identifier) const {
  if (stream_identifier > max_num_streams()) {
    throw std::runtime_error("stream_identifier exceeds maximum value");
  }

  std::unique_lock lock(impl_->mutex);

  return impl_->stream_manager.get(stream_identifier);
}

Stream& Connection::operator[](std::size_t stream_identifier) {
  if (stream_identifier > max_num_streams()) {
    throw std::runtime_error("stream_identifier exceeds maximum value");
  }

  std::unique_lock lock(impl_->mutex);

  return impl_->stream_manager.get(stream_identifier);
}

std::shared_ptr<Connection::ReadyReadEvent> Connection::ready_read() const {
  return impl_->ready_read_event;
}

std::shared_ptr<Connection::StateChangedEvent> Connection::state_changed() const {
  return impl_->state_changed_event;
}

ConnectionPrivate::ConnectionPrivate(asio::io_context& io_context)
    : io_context(io_context),
      strand(io_context),
      in_data_queue(*this),
      out_control_queue(*this),
      out_data_queue(*this),
      ack_manager(*this),
      congestion_manager(*this),
      crypto_manager(*this),
      network_manager(*this),
      packet_builder(*this),
      packet_handler(*this),
      rto_manager(*this),
      state_manager(*this),
      stream_manager(*this),
      timer_manager(*this) {
  reset();
}

ConnectionPrivate::~ConnectionPrivate() = default;

void ConnectionPrivate::reset() {
  in_data_queue.reset();
  out_control_queue.reset();
  out_data_queue.reset();

  ack_manager.reset();
  congestion_manager.reset();
  crypto_manager.reset();
  network_manager.reset();
  packet_builder.reset();
  packet_handler.reset();
  rto_manager.reset();
  state_manager.reset();
  stream_manager.reset();
  timer_manager.reset();
}

}  // namespace protocol
