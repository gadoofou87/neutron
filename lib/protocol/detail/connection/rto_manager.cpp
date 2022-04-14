#include "rto_manager.hpp"

#include <algorithm>

namespace protocol {

namespace detail {

namespace {

constexpr RtoManager::RtoExpDivisor RTO_ALPHA_DEFAULT = 0.125;
constexpr RtoManager::RtoExpDivisor RTO_BETA_DEFAULT = 0.25;
constexpr RtoManager::Rto RTO_INITIAL_DEFAULT = 3000;
constexpr RtoManager::Rto RTO_MAX_DEFAULT = 60000;
constexpr RtoManager::Rto RTO_MIN_DEFAULT = 1000;

}  // namespace

RtoManager::RtoManager(parent_type& parent)
    : utils::Parentable<parent_type>(parent),
      rto_alpha_(RTO_ALPHA_DEFAULT),
      rto_beta_(RTO_BETA_DEFAULT),
      rto_initial_(RTO_INITIAL_DEFAULT),
      rto_max_(RTO_MAX_DEFAULT),
      rto_min_(RTO_MIN_DEFAULT) {}

void RtoManager::backoff_rto() { rto_ = std::min(rto_ * 2, rto_max_); }

RtoManager::Rto RtoManager::rto() const { return rto_; }

RtoManager::RtoExpDivisor RtoManager::rto_alpha() const { return rto_alpha_; }

RtoManager::RtoExpDivisor RtoManager::rto_beta() const { return rto_beta_; }

RtoManager::Rto RtoManager::rto_initial() const { return rto_initial_; }

RtoManager::Rto RtoManager::rto_max() const { return rto_max_; }

RtoManager::Rto RtoManager::rto_min() const { return rto_min_; }

void RtoManager::recalculate(Rtt rtt) {
  if (srtt_ == 0) {
    srtt_ = rtt;
    rttvar_ = rtt / 2;
  } else {
    rttvar_ = (1.F - rto_beta_) * rttvar_ + rto_beta_ * std::abs(srtt_ - rtt);
    srtt_ = (1.F - rto_alpha_) * srtt_ + rto_alpha_ * rtt;
  }

  rto_ = std::clamp<Rto>(srtt_ + 4 * rttvar_, rto_min_, rto_max_);
}

void RtoManager::reset() {
  rto_ = rto_initial_;
  rttvar_ = 0;
  srtt_ = 0;
}

void RtoManager::set_rto_alpha(RtoExpDivisor rto_alpha) { rto_alpha_ = rto_alpha; }

void RtoManager::set_rto_beta(RtoExpDivisor rto_beta) { rto_beta_ = rto_beta; }

void RtoManager::set_rto_initial(Rto rto_initial) { rto_initial_ = rto_initial; }

void RtoManager::set_rto_max(Rto rto_max) { rto_max_ = rto_max; }

void RtoManager::set_rto_min(Rto rto_min) { rto_min_ = rto_min; }

}  // namespace detail

}  // namespace protocol
