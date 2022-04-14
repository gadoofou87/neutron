FetchContent_Declare(
  falcon
  URL https://falcon-sign.info/Falcon-impl-20211101.zip)

FetchContent_GetProperties(falcon)
if(NOT falcon_POPULATED)
  FetchContent_Populate(falcon)
endif()

add_library(falcon
  ${falcon_SOURCE_DIR}/codec.c
  ${falcon_SOURCE_DIR}/common.c
  ${falcon_SOURCE_DIR}/falcon.c
  ${falcon_SOURCE_DIR}/fft.c
  ${falcon_SOURCE_DIR}/fpr.c
  ${falcon_SOURCE_DIR}/keygen.c
  ${falcon_SOURCE_DIR}/rng.c
  ${falcon_SOURCE_DIR}/shake.c
  ${falcon_SOURCE_DIR}/sign.c
  ${falcon_SOURCE_DIR}/vrfy.c)
target_include_directories(falcon PUBLIC ${falcon_SOURCE_DIR})
