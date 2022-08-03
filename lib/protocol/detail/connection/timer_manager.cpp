#include "timer_manager.hpp"

#include <asio/bind_executor.hpp>

#include "api/structures/heartbeat_request.hpp"
#include "connection_p.hpp"
#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

namespace {

constexpr std::chrono::milliseconds DFLT_ACK_INTERVAL{200};
constexpr std::chrono::seconds DFLT_HEARTBEAT_INTERVAL{30};

}  // namespace

void TimerManager::reset() {
  stop_all();

  ack_interval_ = DFLT_ACK_INTERVAL;
  heartbeat_interval_ = DFLT_HEARTBEAT_INTERVAL;
}

template <TimerManager::TimerId Id>
void TimerManager::async_wait_timer_handler(ConnectionPrivate& parent) {
  std::unique_lock lock(parent.mutex);

  const bool restart = parent.timer_manager.handler<Id>();

  parent.network_manager.write_pending_packets();

  if (restart) {
    parent.timer_manager.start<Id>();
  }
}

template <>
bool TimerManager::handler<TimerManager::TimerId::Init>() {
  //

  ASSERT(parent().internal_data.type == Connection::Type::Client);

  ASSERT(parent().internal_data.stored_init.has_value());

  parent().out_control_queue.push(ChunkType::Initiation, *parent().internal_data.stored_init);

  parent().rto_manager.backoff_rto();

  return true;
}

template <>
bool TimerManager::handler<TimerManager::TimerId::Shutdown>() {
  //

  if (parent().state_manager.any_of(Connection::State::ShutdownSent)) {
    parent().out_control_queue.push(ChunkType::ShutdownAssociation, {});
  } else if (parent().state_manager.any_of(Connection::State::ShutdownAckSent)) {
    parent().out_control_queue.push(ChunkType::ShutdownAcknowledgement, {});
  } else {
    ASSERT(false);
  }

  return true;
}

template <>
bool TimerManager::handler<TimerManager::TimerId::Rtx>() {
  ASSERT(parent().out_data_queue.has_inflight());

  //

  parent().out_data_queue.advance_advanced_peer_tsn_ack_point();

  parent().out_data_queue.mark_all_to_retrasmit();

  parent().congestion_manager.on_retransmission();

  parent().rto_manager.backoff_rto();

  parent().network_manager.write_pending_packets();

  return true;
}

template <>
bool TimerManager::handler<TimerManager::TimerId::Ack>() {
  parent().ack_manager.delay_expired();

  return false;
}

template <>
bool TimerManager::handler<TimerManager::TimerId::Heartbeat>() {
  //

  parent().congestion_manager.on_long_idle_period();

  auto buffer = serialization::BufferBuilder<HeartbeatRequest>{}.build();

  HeartbeatRequest heartbeat_request(buffer);

  heartbeat_request.hb_info().time_value = std::chrono::duration_cast<std::chrono::milliseconds>(
                                               std::chrono::steady_clock::now().time_since_epoch())
                                               .count();

  parent().out_control_queue.push(ChunkType::HeartbeatRequest, std::move(buffer));

  parent().rto_manager.backoff_rto();

  return true;
}

template <TimerManager::TimerId Id>
void TimerManager::start_helper(std::chrono::milliseconds expiry_time) {
  auto& timer = timers_[timer_index<Id>()].emplace(parent().io_context, expiry_time);

  timer.async_wait(asio::bind_executor(
      parent().strand, [weak_parent = parent().weak_from_this()](const asio::error_code& error) {
        if (error) {
          return;
        }
        if (auto parent = weak_parent.lock()) [[likely]] {
          async_wait_timer_handler<Id>(*parent);
        }
      }));
}

template <>
void TimerManager::start<TimerManager::TimerId::Init>() {
  start_helper<TimerId::Init>(std::chrono::milliseconds(parent().rto_manager.rto()));
}

template <>
void TimerManager::start<TimerManager::TimerId::Shutdown>() {
  start_helper<TimerId::Shutdown>(std::chrono::milliseconds(parent().rto_manager.rto()));
}

template <>
void TimerManager::start<TimerManager::TimerId::Rtx>() {
  start_helper<TimerId::Rtx>(std::chrono::milliseconds(parent().rto_manager.rto()));
}

template <>
void TimerManager::start<TimerManager::TimerId::Ack>() {
  start_helper<TimerId::Ack>(ack_interval_);
}

template <>
void TimerManager::start<TimerManager::TimerId::Heartbeat>() {
  if (heartbeat_interval_ <= std::chrono::milliseconds::zero()) {
    return;
  }

  // TODO: random jitter
  start_helper<TimerId::Heartbeat>(std::chrono::milliseconds(parent().rto_manager.rto()) +
                                   heartbeat_interval_);
}

void TimerManager::stop_all() {
  for (auto& timer_opt : timers_) {
    timer_opt.reset();
  }
}

}  // namespace detail

}  // namespace protocol
