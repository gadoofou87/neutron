#pragma once

#include "connection.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

template <typename T>
concept StateConcept = std::is_same_v<T, Connection::State>;

class StateManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  template <StateConcept... States>
  bool any_of(States&&... states) const {
    return ((state_ == states) || ...);
  }

  [[nodiscard]] Connection::State get() const;

  template <StateConcept... States>
  bool none_of(States&&... states) const {
    return ((state_ != states) && ...);
  }

  void reset() override;

  void set(Connection::State state);

 private:
  template <Connection::State>
  void handle();

 private:
  Connection::State state_;
};

}  // namespace detail

}  // namespace protocol
