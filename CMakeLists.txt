cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ION_BASE_VERSION "0.1.11")
if (DEFINED ENV{GIT_SHA})
    set(ION_BUILD_VERSION "${ION_BASE_VERSION}-$ENV{GIT_SHA}")
else ()
    set(ION_BUILD_VERSION "${ION_BASE_VERSION}")
endif ()
project(ion VERSION ${ION_BASE_VERSION})

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "The ion project requires Clang. You are trying to use ${CMAKE_CXX_COMPILER_ID}.")
endif ()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
    message(FATAL_ERROR "Clang 17 or higher is required. Found: ${CMAKE_CXX_COMPILER_VERSION}")
endif ()

set(ION_DEV_FLAGS "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ION_DEV_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
endif ()

if (NOT APPLE)
    add_link_options("-fuse-ld=lld")
endif ()

include(FetchContent)

find_package(OpenSSL REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(opentelemetry-cpp CONFIG REQUIRED)

add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(test/unit)
add_subdirectory(test/integration)

enable_testing()
