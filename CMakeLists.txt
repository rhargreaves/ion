cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ION_BASE_VERSION "0.1.14")
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

if (NOT APPLE AND NOT EMSCRIPTEN)
    add_link_options("-fuse-ld=lld")
endif ()

include(FetchContent)

if (ION_WASM)
    set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
            spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(spdlog)
else ()
    find_package(OpenSSL 3 CONFIG REQUIRED COMPONENTS SSL Crypto)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(opentelemetry-cpp CONFIG REQUIRED)
    find_package(CLI11 CONFIG REQUIRED)
    find_package(spdlog CONFIG REQUIRED)
endif ()

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_subdirectory(lib)
if (ION_WASM)
    add_subdirectory(wasm)
else ()
    add_subdirectory(app)
    add_subdirectory(test/unit) # todo: compile for all
    add_subdirectory(test/integration)
endif ()

enable_testing()
