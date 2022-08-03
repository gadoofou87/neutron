set(protobuf_BUILD_TESTS OFF)

FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v21.2
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(protobuf)
