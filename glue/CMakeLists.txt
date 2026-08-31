cmake_minimum_required(VERSION 3.16)
project(libfurble LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB FURBLE_LIB_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/furble/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/blowfish/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/preferences/*.cpp
)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND AND NOT TARGET pakrt)
    list(APPEND CMAKE_PREFIX_PATH "/usr/local")
    pkg_check_modules(LIBPAK IMPORTED_TARGET GLOBAL libpak)
    if(LIBPAK_FOUND)
        add_library(pakrt ALIAS PkgConfig::LIBPAK)
    endif()
endif()

set(FURBLE_LIB_SOURCES ${FURBLE_LIB_SOURCES} nimble.cpp module.cpp)

if(MAKE_EXECUTABLE OR CMAKE_SYSTEM_PROCESSOR STREQUAL "wasm32")
  add_executable(furble ${FURBLE_LIB_SOURCES})
else()
  add_library(furble SHARED ${FURBLE_LIB_SOURCES})
endif()

target_include_directories(furble PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/furble
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/blowfish
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/preferences
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(furble PUBLIC
  CONFIG_BT_NIMBLE_ENABLED=1
)

target_compile_options(furble PRIVATE
  "-include${CMAKE_CURRENT_SOURCE_DIR}/include/furble_host_prefix.h"
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-c++11-narrowing>
  $<$<CXX_COMPILER_ID:GNU>:-Wno-narrowing>
)

target_link_libraries(furble PRIVATE pakrt)
