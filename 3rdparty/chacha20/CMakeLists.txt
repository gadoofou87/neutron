project(chacha20)

add_library(${PROJECT_NAME} chacha.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
