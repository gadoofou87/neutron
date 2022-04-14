#pragma once

namespace utils {

template <typename Parent>
class Parentable {
 protected:
  using parent_type = Parent;

 public:
  explicit Parentable(Parent& parent) : parent_(parent) {}
  virtual ~Parentable() = default;

  const Parent& parent() const noexcept { return parent_; }

  Parent& parent() noexcept { return parent_; }

 private:
  Parent& parent_;
};

}  // namespace utils
