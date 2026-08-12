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

add_library(libfurble SHARED
  nimble.cpp
  module.cpp
  ${FURBLE_LIB_SOURCES}
)

target_include_directories(libfurble PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/furble
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/blowfish
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/preferences
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(libfurble PUBLIC
  CONFIG_BT_NIMBLE_ENABLED=1
)

target_compile_options(libfurble PRIVATE
  "-include${CMAKE_CURRENT_SOURCE_DIR}/include/furble_host_prefix.h"
)

target_link_libraries(libfurble PRIVATE pakrt)
