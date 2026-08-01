cmake_minimum_required(VERSION 3.16)
project(libfurble LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB FURBLE_LIB_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/furble/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/blowfish/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/preferences/*.cpp
)

add_library(libfurble STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/nimble.cpp
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

# Host build: pull in ESP-IDF/FreeRTOS shims without editing furble sources.
target_compile_options(libfurble PRIVATE
  "-include${CMAKE_CURRENT_SOURCE_DIR}/include/furble_host_prefix.h"
)
