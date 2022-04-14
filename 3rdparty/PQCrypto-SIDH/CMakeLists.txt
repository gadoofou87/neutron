FetchContent_Declare(
  PQCrypto-SIDH
  GIT_REPOSITORY https://github.com/microsoft/PQCrypto-SIDH.git
  GIT_TAG v3.4
  GIT_SHALLOW TRUE)

FetchContent_GetProperties(PQCrypto-SIDH)
if(NOT PQCrypto-SIDH_POPULATED)
  FetchContent_Populate(PQCrypto-SIDH)
endif()

add_library(
  sidhp434_compressed ${pqcrypto-sidh_SOURCE_DIR}/src/P434/P434_compressed.c
                      ${pqcrypto-sidh_SOURCE_DIR}/src/P434/generic/fp_generic.c)
target_include_directories(sidhp434_compressed
                           PUBLIC ${pqcrypto-sidh_SOURCE_DIR}/src/P434)
target_link_libraries(sidhp434_compressed PRIVATE randombytes fips202)

if(WIN32)
  target_compile_definitions(sidhp434_compressed PRIVATE __WINDOWS__)
elseif(UNIX)
  target_compile_definitions(sidhp434_compressed PRIVATE __NIX__)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
  target_compile_definitions(sidhp434_compressed PRIVATE _AMD64_)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armel|armhf|armv7|arm32v7")
  target_compile_definitions(sidhp434_compressed PRIVATE _ARM_)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|arm64v8")
  target_compile_definitions(sidhp434_compressed PRIVATE _ARM64_)
endif()

target_compile_definitions(sidhp434_compressed PRIVATE _GENERIC_)
