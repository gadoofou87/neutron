#pragma once

#include "../types/exception_code.hpp"
#include "../types/request_identifier.hpp"
#include "serialization/buffer_builder.hpp"
#include "serialization/packed_integer.hpp"
#include "serialization/packed_struct.hpp"

namespace communication {

namespace detail {

class Exception : public serialization::PackedStruct {
 public:
  using PackedStruct::PackedStruct;

  auto& id() { return jmp_ref<id_type>(id_offset()); }
  auto& code() { return jmp_ref<code_type>(code_offset()); }

  bool validate() override {
    if (!range_check(id_offset(), sizeof(id_type))) {
      return false;
    }
    if (!range_check(code_offset(), sizeof(code_type))) {
      return false;
    }

    return true;
  }

 public:
  using id_type = serialization::PackedInteger<RequestIdentifier>;
  using code_type = serialization::PackedInteger<ExceptionCode>;

 private:
  size_t id_offset() { return 0; }
  size_t code_offset() { return id_offset() + sizeof(id_type); }
};

}  // namespace detail

}  // namespace communication

namespace serialization {

using namespace communication::detail;

template <typename... Tags>
class BufferBuilder<Exception, Tags...> {
 public:
  BufferBuilder() = default;

  auto build() { return std::vector<uint8_t>(buffer_size()); }

  size_t buffer_size() { return sizeof(Exception::id_type) + sizeof(Exception::code_type); }
};

}  // namespace serialization
