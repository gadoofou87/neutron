set(SKIP_BUILD_TEST TRUE)

FetchContent_Declare(
  libpqxx
  GIT_REPOSITORY https://github.com/jtv/libpqxx.git
  GIT_TAG 7.7.3
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(libpqxx)
