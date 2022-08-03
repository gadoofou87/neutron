#include "network_manager.hpp"

#include "connection_p.hpp"
#include "detail/async_recursive_read_datagram.hpp"
#include "detail/async_send_datagram.hpp"
#include "detail/connection/api/structures/packet.hpp"
#include "detail/send_datagram.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

bool NetworkManager::is_receiving() const { return receiving_; }

void NetworkManager::reset() {
  receiving_ = false;
  rx_socket_.reset();
  tx_endpoint_.reset();
  tx_socket_.reset();
}

void NetworkManager::set_rx_socket(
    std::shared_ptr<asio::generic::datagram_protocol::socket> socket) {
  if (socket != nullptr) {
    ASSERT(socket->is_open());
  }

  receiving_ = false;
  rx_socket_ = std::move(socket);
}

void NetworkManager::set_tx_endpoint(
    std::optional<asio::generic::datagram_protocol::endpoint> endpoint) {
  tx_endpoint_ = std::move(endpoint);
}

void NetworkManager::set_tx_socket(
    std::shared_ptr<asio::generic::datagram_protocol::socket> socket) {
  if (socket != nullptr) {
    ASSERT(socket->is_open());
  }

  tx_socket_ = std::move(socket);
}

void NetworkManager::start_receive() {
  ASSERT(receiving_ == false);
  ASSERT(rx_socket_ != nullptr);

  async_recursive_read_datagram(
      parent().strand, rx_socket_, [weak_parent = parent().weak_from_this()](auto&&... args) {
        if (auto parent = weak_parent.lock()) [[likely]] {
          async_receive_rx_socket_handler(*parent, std::forward<decltype(args)>(args)...);
        }
      });

  receiving_ = true;
}

void NetworkManager::stop_receive() {
  ASSERT(receiving_ == true);
  ASSERT(rx_socket_ != nullptr);

  rx_socket_->cancel();

  receiving_ = false;
}

template <>
void NetworkManager::write_pending_packets<false>() {
  ASSERT(tx_socket_ != nullptr);

  auto packets = gather_outbound();

  for (auto& packet : packets) {
    send_datagram<asio::generic::datagram_protocol>(*tx_socket_, tx_endpoint_, std::move(packet));
  }
}

template <>
void NetworkManager::write_pending_packets<true>() {
  ASSERT(tx_socket_ != nullptr);

  auto packets = gather_outbound();

  for (auto& packet : packets) {
    async_send_datagram<asio::generic::datagram_protocol>(*tx_socket_, tx_endpoint_,
                                                          std::move(packet));
  }
}

void NetworkManager::async_receive_rx_socket_handler(
    ConnectionPrivate& parent, std::vector<uint8_t> data,
    asio::generic::datagram_protocol::endpoint endpoint) {
  std::unique_lock lock(parent.mutex);

  if (parent.network_manager.tx_socket_->native_handle() ==
      parent.network_manager.rx_socket_->native_handle()) {
    parent.network_manager.tx_endpoint_ = std::move(endpoint);
  }

  if (!parent.packet_handler.handle(Packet(data))) {
    return;
  }
}

std::list<std::vector<uint8_t>> NetworkManager::gather_outbound() {
  std::list<std::vector<uint8_t>> result;

  result.splice(result.cend(), parent().out_control_queue.gather_unsent_packets());

  if (parent().state_manager.any_of(Connection::State::Established,
                                    Connection::State::ShutdownPending,
                                    Connection::State::ShutdownReceived)) [[likely]] {
    const auto a = result.size();

    result.splice(result.cend(), parent().out_data_queue.gather_fast_retransmission_packets());
    result.splice(result.cend(), parent().out_data_queue.gather_packets_to_retransmit());
    result.splice(result.cend(), parent().out_data_queue.gather_unsent_packets());

    const auto b = result.size();

    if (a != b && !parent().timer_manager.is_started<TimerManager::TimerId::Rtx>()) {
      parent().timer_manager.stop<TimerManager::TimerId::Heartbeat>();
      parent().timer_manager.start<TimerManager::TimerId::Rtx>();
    }
  }

  return result;
}

}  // namespace detail

}  // namespace protocol
