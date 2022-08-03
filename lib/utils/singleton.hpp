#pragma once

namespace utils {

template <typename T>
class Singleton {
 public:
  static T& instance() {
    static T instance(Token{});
    return instance;
  }

 protected:
  struct Token {};

  Singleton() = default;
};

}  // namespace utils
