cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ion VERSION 0.1.2)
set(ION_BUILD_VERSION "${PROJECT_VERSION}")

# Link libc++abi on Linux
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
