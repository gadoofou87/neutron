project(randombytes)

add_library(${PROJECT_NAME} randombytes.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
