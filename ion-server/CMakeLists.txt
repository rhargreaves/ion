cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
project(ion-server)

include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.16.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)
find_package(OpenSSL REQUIRED)

add_executable(ion-server main.cpp
        tls_conn.cpp
        tls_conn.h
        http2_frames.h
        http2_conn.cpp
        http2_conn.h
        http2_conn.cpp
        tls_conn_except.cpp
        tls_conn_except.h
        http2_except.h
        socket_fd.h
        tcp_conn.cpp
        tcp_conn.h
)

target_link_libraries(ion-server PRIVATE
        OpenSSL::SSL OpenSSL::Crypto
        spdlog::spdlog
)

target_compile_options(ion-server PRIVATE
        -Wall -Wextra -Wpedantic -Werror
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ion-server PRIVATE
            -fsanitize=address
            -fno-omit-frame-pointer
            -g
    )
    target_link_options(ion-server PRIVATE
            -fsanitize=address
    )
endif ()
