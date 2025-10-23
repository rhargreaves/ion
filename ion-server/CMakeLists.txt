cmake_minimum_required(VERSION 4.0)
project(ion-server)
set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)

add_executable(ion-server main.cpp
        tls_conn.cpp
        tls_conn.h
        http2_frames.h
        http2_conn.cpp
        http2_conn.h
        http2_conn.cpp
        tls_conn_except.cpp
        tls_conn_except.h)

target_link_libraries(ion-server PRIVATE OpenSSL::SSL OpenSSL::Crypto)

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
