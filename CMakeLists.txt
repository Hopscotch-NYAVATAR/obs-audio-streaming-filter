cmake_minimum_required(VERSION 3.16...3.26)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
                                  -Wno-comma>)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON)
endif()

if(OS_LINUX)
  set(TRIPLET x64-linux)
elseif(OS_MACOS)
  set(TRIPLET universal-osx)
elseif(OS_WINDOWS)
  set(TRIPLET x64-windows-static)
endif()

find_package(Opus CONFIG REQUIRED HINTS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET})
find_path(
  LIBOPUSENC_INCLUDE_DIR
  NAMES opusenc.h
  PATH_SUFFIXES opus
  PATHS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET}/include REQUIRED
  NO_DEFAULT_PATH)
find_library(
  LIBOPUSENC_LIBRARIES opusenc
  PATHS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET}/lib REQUIRED
  NO_DEFAULT_PATH)
message(FATAL_ERROR ${LIBOPUSENC_LIBRARIES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${LIBOPUSENC_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Opus::opus ${LIBOPUSENC_LIBRARIES})

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/RecordPathGenerator.hpp
          src/AudioStreamingFilterContext.h
          src/AudioStreamingFilterContext.cpp
          src/audio-streaming-filter.h
          src/audio-streaming-filter.cpp
          src/audio-streaming-filter-info.c
          src/plugin-main.c)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
