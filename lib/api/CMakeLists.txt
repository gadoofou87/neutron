project(api)

file(GLOB_RECURSE HEADERS *.hpp)

add_library(${PROJECT_NAME} INTERFACE ${HEADERS})
