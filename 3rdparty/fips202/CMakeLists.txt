project(fips202)

add_library(${PROJECT_NAME} fips202.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
