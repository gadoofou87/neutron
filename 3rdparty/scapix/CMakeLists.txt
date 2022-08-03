FetchContent_Declare(
  cmodule
  GIT_REPOSITORY https://github.com/scapix-com/cmodule.git
  GIT_TAG v1.0.35
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(cmodule)

find_package(Scapix REQUIRED)
