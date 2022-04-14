#include "state_manager.hpp"

#include "connection_p.hpp"

namespace protocol {

namespace detail {

Connection::State StateManager::get() const { return state_; }

void StateManager::reset() { state_ = Connection::State::Closed; }

template <>
void StateManager::handle<Connection::State::Closed>() {
  parent().network_manager.stop_receive();
  parent().timer_manager.stop_all();

  parent().self.reset();
}

template <>
void StateManager::handle<Connection::State::Listen>() {}

template <>
void StateManager::handle<Connection::State::InitSent>() {
  parent().self = parent().shared_from_this();
}

template <>
void StateManager::handle<Connection::State::InitReceived>() {
  parent().self = parent().shared_from_this();
}

template <>
void StateManager::handle<Connection::State::Established>() {
  parent().network_manager.write_pending_packets();

  if (parent().out_data_queue.empty()) {
    parent().timer_manager.start<TimerManager::TimerId::Heartbeat>();
  }
}

template <>
void StateManager::handle<Connection::State::ShutdownPending>() {}

template <>
void StateManager::handle<Connection::State::ShutdownSent>() {
  parent().timer_manager.stop<TimerManager::TimerId::Heartbeat>();
  parent().timer_manager.start<TimerManager::TimerId::Shutdown>();
}

template <>
void StateManager::handle<Connection::State::ShutdownReceived>() {}

template <>
void StateManager::handle<Connection::State::ShutdownAckSent>() {
  parent().timer_manager.stop<TimerManager::TimerId::Heartbeat>();
  parent().timer_manager.start<TimerManager::TimerId::Shutdown>();
}

void StateManager::set(Connection::State state) {
  state_ = state;

  switch (state_) {
    case Connection::State::Closed:
      handle<Connection::State::Closed>();
      break;
    case Connection::State::Listen:
      handle<Connection::State::Listen>();
      break;
    case Connection::State::InitSent:
      handle<Connection::State::InitSent>();
      break;
    case Connection::State::InitReceived:
      handle<Connection::State::InitReceived>();
      break;
    case Connection::State::Established:
      handle<Connection::State::Established>();
      break;
    case Connection::State::ShutdownPending:
      handle<Connection::State::ShutdownPending>();
      break;
    case Connection::State::ShutdownSent:
      handle<Connection::State::ShutdownSent>();
      break;
    case Connection::State::ShutdownReceived:
      handle<Connection::State::ShutdownReceived>();
      break;
    case Connection::State::ShutdownAckSent:
      handle<Connection::State::ShutdownAckSent>();
      break;
  }

  parent().events.state_changed->emit(state_);
}

}  // namespace detail

}  // namespace protocol
