FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-22-1
  GIT_SHALLOW TRUE)

FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
  FetchContent_Populate(asio)
endif()

file(WRITE ${asio_BINARY_DIR}/src.cpp "#include <asio/impl/src.hpp>")

add_library(asio ${asio_BINARY_DIR}/src.cpp)
target_compile_definitions(
  asio PRIVATE ASIO_NO_DEPRECATED ASIO_SEPARATE_COMPILATION ASIO_STANDALONE)
target_include_directories(asio PUBLIC ${asio_SOURCE_DIR}/asio/include)
target_link_libraries(asio PRIVATE Threads::Threads)
