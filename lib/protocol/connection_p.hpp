#pragma once

#include <asio/io_context_strand.hpp>
#include <shared_mutex>

#include "connection.hpp"
#include "detail/connection/ack_manager.hpp"
#include "detail/connection/congestion_manager.hpp"
#include "detail/connection/crypto_manager.hpp"
#include "detail/connection/in_data_queue.hpp"
#include "detail/connection/internal_data.hpp"
#include "detail/connection/network_manager.hpp"
#include "detail/connection/out_control_queue.hpp"
#include "detail/connection/out_data_queue.hpp"
#include "detail/connection/packet_builder.hpp"
#include "detail/connection/packet_handler.hpp"
#include "detail/connection/rto_manager.hpp"
#include "detail/connection/state_manager.hpp"
#include "detail/connection/stream_manager.hpp"
#include "detail/connection/timer_manager.hpp"
#include "utils/abstract/iresetable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate : public std::enable_shared_from_this<ConnectionPrivate>,
                          utils::IResetable {
 public:
  explicit ConnectionPrivate(asio::io_context& io_context);
  virtual ~ConnectionPrivate();

 public:
  void reset() override;

 public:
  asio::io_context& io_context;
  asio::io_context::strand strand;
  std::recursive_mutex mutex;

  InternalData internal_data;

  InDataQueue in_data_queue;
  OutControlQueue out_control_queue;
  OutDataQueue out_data_queue;

  AckManager ack_manager;
  CongestionManager congestion_manager;
  CryptoManager crypto_manager;
  NetworkManager network_manager;
  PacketBuilder packet_builder;
  PacketHandler packet_handler;
  RtoManager rto_manager;
  StateManager state_manager;
  StreamManager stream_manager;
  TimerManager timer_manager;

  std::shared_ptr<Connection::ReadyReadEvent> ready_read_event;
  std::shared_ptr<Connection::StateChangedEvent> state_changed_event;

 public:
  std::shared_ptr<ConnectionPrivate> self;
};

}  // namespace detail

}  // namespace protocol
