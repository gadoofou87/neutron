project(serialization)

file(GLOB_RECURSE HEADERS *.hpp)

add_library(${PROJECT_NAME} INTERFACE ${HEADERS})
