add_library(ion
        tls_conn.cpp
        tls_conn.h
        http2_frames.h
        http2_conn.cpp
        http2_conn.h
        http2_conn.cpp
        socket_fd.h
        tcp_conn.cpp
        tcp_conn.h
        http2_server.cpp
        http2_server.h
        args.cpp
        args.h
        header_block_decoder.cpp
        header_block_decoder.h
        huffman_tree.cpp
        huffman_tree.h
        huffman_codes.h
)

target_include_directories(ion PUBLIC .)
target_link_libraries(ion PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        CLI11::CLI11
)

add_executable(ion-server main.cpp)
target_link_libraries(ion-server PRIVATE ion)

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
