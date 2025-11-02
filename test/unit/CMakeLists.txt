cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(ion-unit-tests)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

find_package(OpenSSL REQUIRED)

# Fetch other dependencies (same as main project)
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11
        GIT_TAG v2.4.1
)
FetchContent_MakeAvailable(CLI11)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

add_executable(unit-tests
        test_hpack.cpp
        ../../ion-server/http2_conn.cpp
        ../../ion-server/tcp_conn.cpp
        ../../ion-server/tls_conn.cpp
        ../../ion-server/http2_server.cpp
)

target_link_libraries(unit-tests
        PRIVATE
        Catch2::Catch2WithMain
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        CLI11::CLI11
)

target_include_directories(unit-tests PRIVATE ../../ion-server)
enable_testing()

# Register tests with CTest
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(unit-tests)
