include(FetchContent)

set(CMAKE_UNITY_BUILD OFF)

if(BUILD_TESTING)
  add_subdirectory(ut)

  set(BUILD_TESTING OFF)
endif()

add_subdirectory(chacha20)
add_subdirectory(fips202)
add_subdirectory(randombytes)

add_subdirectory(argon2)
add_subdirectory(asio)
add_subdirectory(falcon)
add_subdirectory(fmt)
add_subdirectory(poly1305-donna)
add_subdirectory(PQCrypto-SIDH)
add_subdirectory(spdlog)

if(BUILD_SERVER)
  add_subdirectory(cxxopts)
  add_subdirectory(mdbx)
  add_subdirectory(tomlplusplus)
endif()
