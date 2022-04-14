FetchContent_Declare(
  poly1305-donna
  GIT_REPOSITORY https://github.com/floodyberry/poly1305-donna.git
  GIT_TAG e6ad6e091d30d7f4ec2d4f978be1fcfcbce72781
  GIT_SHALLOW TRUE)

FetchContent_GetProperties(poly1305-donna)
if(NOT poly1305-donna_POPULATED)
  FetchContent_Populate(poly1305-donna)
endif()

add_library(poly1305-donna ${poly1305-donna_SOURCE_DIR}/poly1305-donna.c)
target_include_directories(poly1305-donna PUBLIC ${poly1305-donna_SOURCE_DIR})
