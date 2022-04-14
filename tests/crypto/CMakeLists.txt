link_libraries(crypto)

add_executable(test_chacha20poly1305 test_chacha20poly1305.cpp)
add_test(NAME test_chacha20poly1305 COMMAND test_chacha20poly1305)

add_executable(test_sha3_256 test_sha3_256.cpp)
add_test(NAME test_sha3_256 COMMAND test_sha3_256)

add_executable(test_sha3_384 test_sha3_384.cpp)
add_test(NAME test_sha3_384 COMMAND test_sha3_384)

add_executable(test_sha3_512 test_sha3_512.cpp)
add_test(NAME test_sha3_512 COMMAND test_sha3_512)

add_executable(test_shake128 test_shake128.cpp)
add_test(NAME test_shake128 COMMAND test_shake128)

add_executable(test_shake256 test_shake256.cpp)
add_test(NAME test_shake256 COMMAND test_shake256)

add_executable(test_sidhp434_compressed test_sidhp434_compressed.cpp)
add_test(NAME test_sidhp434_compressed COMMAND test_sidhp434_compressed)

add_executable(test_xchacha20poly1305 test_xchacha20poly1305.cpp)
add_test(NAME test_xchacha20poly1305 COMMAND test_xchacha20poly1305)
