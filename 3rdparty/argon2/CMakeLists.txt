FetchContent_Declare(
  argon2
  GIT_REPOSITORY https://github.com/P-H-C/phc-winner-argon2.git
  GIT_TAG f57e61e19229e23c4445b85494dbf7c07de721cb
  GIT_SHALLOW TRUE)

FetchContent_GetProperties(argon2)
if(NOT argon2_POPULATED)
  FetchContent_Populate(argon2)
endif()

add_library(argon2
    ${argon2_SOURCE_DIR}/src/blake2/blake2b.c
    ${argon2_SOURCE_DIR}/src/argon2.c
    ${argon2_SOURCE_DIR}/src/core.c
    ${argon2_SOURCE_DIR}/src/encoding.c
    ${argon2_SOURCE_DIR}/src/ref.c
    ${argon2_SOURCE_DIR}/src/thread.c)
target_include_directories(argon2 PUBLIC ${argon2_SOURCE_DIR}/include)
target_link_libraries(argon2 PRIVATE Threads::Threads)
