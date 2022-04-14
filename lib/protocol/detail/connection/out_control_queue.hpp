#pragma once

#include <cstdint>
#include <list>
#include <vector>

#include "api/structures/chunk.hpp"
#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

struct OutControlQueue : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  struct StorageValue {
    ChunkType type;
    std::vector<uint8_t> data;
  };

 public:
  using storage_type = std::list<StorageValue>;

 public:
  using Parentable::Parentable;

  void clear();

  [[nodiscard]] bool empty() const;

  std::list<std::vector<uint8_t>> gather_unsent_packets();

  void push(ChunkType type, std::vector<uint8_t> data);

  void reset() override;

 private:
  storage_type storage_;
};

}  // namespace detail

}  // namespace protocol
