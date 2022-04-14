#pragma once

namespace utils {

class IResetable {
 protected:
  ~IResetable() = default;

  virtual void reset() = 0;
};

}  // namespace utils
