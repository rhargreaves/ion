cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ION_BASE_VERSION "0.1.7")
if (DEFINED ENV{GIT_SHA})
    set(ION_BUILD_VERSION "${ION_BASE_VERSION}-$ENV{GIT_SHA}")
else ()
    set(ION_BUILD_VERSION "${ION_BASE_VERSION}")
endif ()
project(ion VERSION ${ION_BASE_VERSION})

set(ION_DEV_FLAGS "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND ION_DEV_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
endif ()

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>")
add_link_options("$<$<COMPILE_LANGUAGE:CXX>:-stdlib=libc++>")

if (UNIX AND NOT APPLE)
    link_libraries(c++abi)
endif ()

find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.6.1
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CLI11)

add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(test/unit)
add_subdirectory(test/integration)

enable_testing()
