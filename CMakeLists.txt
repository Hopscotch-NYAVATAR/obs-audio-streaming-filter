cmake_minimum_required(VERSION 3.16...3.26)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET plugin-support PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
if(OS_WINDOWS)
  get_property(
    OPUS_LIBRARIES_RELEASE
    TARGET Opus::opus
    PROPERTY IMPORTED_LOCATION_RELEASE)
  set_target_properties(Opus::opus PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO ${OPUS_LIBRARIES_RELEASE}
                                              IMPORTED_LOCATION_MINSIZEREL ${OPUS_LIBRARIES_RELEASE})
endif()

find_path(
  LIBOPUSENC_INCLUDE_DIR
  NAMES opusenc.h
  PATH_SUFFIXES opus
  PATHS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET}/include REQUIRED
  NO_DEFAULT_PATH)

find_library(
  LIBOPUSENC_LIBRARIES_RELEASE opusenc
  PATHS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET}/lib REQUIRED
  PATH_SUFFIXES "lib"
  NO_DEFAULT_PATH)

find_library(
  LIBOPUSENC_LIBRARIES_DEBUG opusenc
  PATHS ${CMAKE_SOURCE_DIR}/vcpkg_installed/${TRIPLET}/lib REQUIRED
  PATH_SUFFIXES "debug"
  NO_DEFAULT_PATH)

add_library(Opusenc STATIC IMPORTED)

set_target_properties(Opusenc PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${LIBOPUSENC_INCLUDE_DIR})
if(OS_WINDOWS)
  set_target_properties(
    Opusenc
    PROPERTIES IMPORTED_LOCATION_DEBUG ${LIBOPUSENC_LIBRARIES_DEBUG}
               IMPORTED_LOCATION_RELEASE ${LIBOPUSENC_LIBRARIES_RELEASE}
               IMPORTED_LOCATION_RELWITHDEBINFO ${LIBOPUSENC_LIBRARIES_RELEASE}
               IMPORTED_LOCATION_MINSIZEREL ${LIBOPUSENC_LIBRARIES_RELEASE})
else()
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBOPUSENC_LIBRARIES ${LIBOPUSENC_LIBRARIES_DEBUG})
  else()
    set(LIBOPUSENC_LIBRARIES ${LIBOPUSENC_LIBRARIES_RELEASE})
  endif()
  set_target_properties(Opusenc PROPERTIES IMPORTED_LOCATION ${LIBOPUSENC_LIBRARIES})
endif()

find_package(CURL REQUIRED)

find_package(Curlpp REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Opus::opus Opusenc CURL::libcurl Curlpp::curlpp)

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
