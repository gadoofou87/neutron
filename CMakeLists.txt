cmake_minimum_required(VERSION 3.10)

project(neutron)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "" ON)
option(BUILD_CONTROLLER "" ON)
option(BUILD_SERVER "" ON)
option(BUILD_TESTING "" OFF)

if(BUILD_TESTING)
  enable_testing(true)
endif()

if(NOT BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS OFF)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT CMAKE_UNITY_BUILD)
  #set(CMAKE_UNITY_BUILD ON)
endif()

find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

find_package(Threads)

add_subdirectory(3rdparty)
add_subdirectory(api)

add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wpedantic)
add_compile_options(-Werror)
add_compile_options(-Wno-unused-parameter)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  add_compile_options(-g)
  add_compile_options(-fno-omit-frame-pointer)

  if("Address" IN_LIST USE_SANITIZER)
    add_compile_options(-fno-optimize-sibling-calls)
    add_compile_options(-fsanitize-address-use-after-scope)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
  if("Thread" IN_LIST USE_SANITIZER)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
  endif()
  if("Undefined" IN_LIST USE_SANITIZER)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
  if("Leak" IN_LIST USE_SANITIZER)
    add_compile_options(-fsanitize=leak)
    add_link_options(-fsanitize=leak)
  endif()
  if("Memory" IN_LIST USE_SANITIZER)
    add_compile_options(-fsanitize=memory)
    add_link_options(-fsanitize=memory)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_RESULT OUTPUT IPO_OUTPUT)

  if(IPO_RESULT)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(WARNING "IPO is not supported: ${IPO_OUTPUT}")
  endif()

  add_compile_options(-O3)
endif()

include_directories(lib)

add_subdirectory(lib)

if(BUILD_CLIENT)
  add_subdirectory(client)
endif()

if(BUILD_CONTROLLER)
  add_subdirectory(controller)
endif()

if(BUILD_SERVER)
  add_subdirectory(server)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
