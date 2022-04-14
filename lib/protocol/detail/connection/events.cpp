#include "events.hpp"

namespace protocol {

namespace detail {

Events::Events()
    : ready_read(Connection::ReadyReadEvent::create()),
      state_changed(Connection::StateChangedEvent::create()) {}

Events::~Events() = default;

}  // namespace detail

}  // namespace protocol
