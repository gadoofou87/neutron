#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "utils/debug/assert.hpp"

namespace protocol {

namespace detail {

template <typename Counter, typename Block = uint64_t, size_t RingBlocks = 128>
class AntiReplayWindow {
 private:
  enum : size_t {
    BlockBits = std::numeric_limits<Block>::digits,
    BlockBitLog = std::bit_width(BlockBits - 1),
    WindowSize = (RingBlocks - 1) * BlockBits,
    BlockMask = RingBlocks - 1,
    BitMask = BlockBits - 1
  };

 public:
  [[nodiscard]] bool check(Counter counter) const {
    if (counter > last_) {
      return true;
    }
    if (last_ - counter > WindowSize) {
      return false;
    }

    auto index = (counter >> BlockBitLog) & BlockMask;
    auto bit_location = counter & BitMask;

    return (ring_[index] & (static_cast<Block>(1) << bit_location)) == 0;
  }

  void reset() {
    last_ = 0;
    ring_[0] = 0;
  }

  void update(Counter counter) {
    auto index = counter >> BlockBitLog;

    if (counter > last_) {
      auto current = last_ >> BlockBitLog;
      auto diff = index - current;

      if (diff > RingBlocks) {
        diff = RingBlocks;
      }

      for (auto i = current + 1; i <= current + diff; ++i) {
        ring_[i & BlockMask] = 0;
      }

      last_ = counter;
    } else {
      ASSERT(last_ - counter <= WindowSize);
    }

    index &= BlockMask;

    auto bit_location = counter & BitMask;

    ASSERT((ring_[index] & (static_cast<Block>(1) << bit_location)) == 0);

    ring_[index] |= (static_cast<Block>(1) << bit_location);
  }

 private:
  Counter last_;
  std::array<Block, RingBlocks> ring_;
};

}  // namespace detail

}  // namespace protocol
