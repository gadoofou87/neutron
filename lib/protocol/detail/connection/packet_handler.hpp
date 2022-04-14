#pragma once

#include "api/types/transmission_sequence_number.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class Packet;

class PacketHandler : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using Parentable::Parentable;

  bool handle(Packet packet);

  void reset() override;

 private:
  template <typename T>
  void handle(T);
};

}  // namespace detail

}  // namespace protocol
