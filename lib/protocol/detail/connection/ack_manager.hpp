#pragma once

#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class AckManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  enum class State { Delay, Idle };

 public:
  using Parentable::Parentable;

  void commit();

  void delay_expired();

  void reset() override;

  void trigger_delayed_ack();

  void trigger_immediate_ack();

 private:
  void send_selective_ack();

 private:
  bool delayed_ack_triggered_;
  bool immediate_ack_triggered_;
  State state_;
};

}  // namespace detail

}  // namespace protocol
