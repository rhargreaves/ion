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
        headers/header_block_decoder.cpp
        headers/header_block_decoder.h
        headers/huffman_tree.cpp
        headers/huffman_tree.h
        headers/huffman_codes.h
        headers/header_static_table.h
        headers/byte_reader.h
        headers/http_header.h
        headers/static_http_header.h
        headers/dynamic_table.cpp
        headers/dynamic_table.h
)

target_include_directories(ion PUBLIC .)
target_link_libraries(ion PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
)

target_compile_features(ion PUBLIC cxx_std_23)
