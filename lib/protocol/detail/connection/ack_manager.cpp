#include "ack_manager.hpp"

#include "api/structures/selective_acknowledgement.hpp"
#include "connection_p.hpp"
#include "utils/span/copy.hpp"

namespace protocol {

namespace detail {

void AckManager::commit() {
  if (state_ == State::Delay || immediate_ack_triggered_) {
    state_ = State::Idle;

    parent().timer_manager.stop<TimerManager::TimerId::Ack>();

    send_selective_ack();
  } else if (delayed_ack_triggered_) {
    state_ = State::Delay;

    parent().timer_manager.start<TimerManager::TimerId::Ack>();
  }

  delayed_ack_triggered_ = false;
  immediate_ack_triggered_ = false;
}

void AckManager::delay_expired() {
  if (state_ != State::Delay) {
    return;
  }

  state_ = State::Idle;

  send_selective_ack();
}

void AckManager::reset() {
  delayed_ack_triggered_ = false;
  immediate_ack_triggered_ = false;
  state_ = State::Idle;
}

void AckManager::trigger_delayed_ack() { delayed_ack_triggered_ = true; }

void AckManager::trigger_immediate_ack() { immediate_ack_triggered_ = true; }

void AckManager::send_selective_ack() {
  const auto gap_ack_blocks = parent().in_data_queue.get_gap_ack_blocks();

  auto buffer = serialization::BufferBuilder<SelectiveAcknowledgement>{}
                    .set_num_gap_ack_blocks(gap_ack_blocks.size())
                    .build();

  SelectiveAcknowledgement selective_ack(buffer);

  selective_ack.cum_tsn_ack() = parent().in_data_queue.peer_last_tsn();

  if (!gap_ack_blocks.empty()) {
    utils::span::copy<GapAckBlock>(selective_ack.gap_ack_blks(), gap_ack_blocks);
  }

  parent().out_control_queue.push(ChunkType::SelectiveAcknowledgement, std::move(buffer));
}

}  // namespace detail

}  // namespace protocol
