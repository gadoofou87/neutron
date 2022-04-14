if(BUILD_SHARED_LIBS)
  set(MDBX_BUILD_SHARED_LIBRARY ON)
else()
  set(MDBX_INSTALL_STATIC ON)
endif()

set(MDBX_BUILD_TOOLS OFF)

FetchContent_Declare(
  mdbx
  URL https://github.com/erthink/libmdbx/releases/download/v0.11.6/libmdbx-amalgamated-0_11_6.zip)

FetchContent_MakeAvailable(mdbx)

add_library(mdbx-interface INTERFACE)
target_include_directories(mdbx-interface INTERFACE ${mdbx_SOURCE_DIR})

if(BUILD_SHARED_LIBS)
  target_link_libraries(mdbx-interface INTERFACE mdbx)
else()
  target_link_libraries(mdbx-interface INTERFACE mdbx-static)
endif()
