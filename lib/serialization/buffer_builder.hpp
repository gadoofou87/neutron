#pragma once

#include <cstddef>

namespace serialization {

template <size_t I>
struct BufferBuilderTag {};

template <typename T, typename... Tags>
class BufferBuilder {
 public:
  BufferBuilder() = delete;
};

}  // namespace serialization
