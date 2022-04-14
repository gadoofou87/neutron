#pragma once

#include <cstdint>

#include "utils/abstract/iresetable.hpp"
#include "utils/parentable.hpp"

namespace protocol {

namespace detail {

class ConnectionPrivate;

class RtoManager : public utils::Parentable<ConnectionPrivate>, utils::IResetable {
 public:
  using RtoExpDivisor = float;
  using Rto = uint32_t;
  using Rtt = uint32_t;
  using RttVar = float;
  using Srtt = float;

 public:
  explicit RtoManager(parent_type& parent);

  void backoff_rto();

  [[nodiscard]] Rto rto() const;

  [[nodiscard]] RtoExpDivisor rto_alpha() const;

  [[nodiscard]] RtoExpDivisor rto_beta() const;

  [[nodiscard]] Rto rto_initial() const;

  [[nodiscard]] Rto rto_max() const;

  [[nodiscard]] Rto rto_min() const;

  void recalculate(Rtt rtt);

  void reset() override;

  void set_rto_alpha(RtoExpDivisor rto_alpha);

  void set_rto_beta(RtoExpDivisor rto_beta);

  void set_rto_initial(Rto rto_initial);

  void set_rto_max(Rto rto_max);

  void set_rto_min(Rto rto_min);

 private:
  Rto rto_;
  RtoExpDivisor rto_alpha_;
  RtoExpDivisor rto_beta_;
  Rto rto_initial_;
  Rto rto_max_;
  Rto rto_min_;
  RttVar rttvar_;
  Srtt srtt_;
};

}  // namespace detail

}  // namespace protocol
