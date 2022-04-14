FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.0.1
  GIT_SHALLOW TRUE)

FetchContent_GetProperties(tomlplusplus)
if(NOT tomlplusplus_POPULATED)
  FetchContent_Populate(tomlplusplus)
endif()

file(WRITE ${tomlplusplus_BINARY_DIR}/src.cpp "#include <toml.hpp>")

add_library(tomlplusplus ${tomlplusplus_BINARY_DIR}/src.cpp)
target_compile_definitions(tomlplusplus PRIVATE TOML_IMPLEMENTATION)
target_compile_definitions(tomlplusplus PUBLIC TOML_ENABLE_FORMATTERS=0 TOML_HEADER_ONLY=0)
target_include_directories(tomlplusplus PRIVATE ${tomlplusplus_SOURCE_DIR})
target_include_directories(tomlplusplus PUBLIC ${tomlplusplus_SOURCE_DIR}/include)
