configure_file(
        ${CMAKE_SOURCE_DIR}/lib/version.h.in
        ${CMAKE_BINARY_DIR}/generated/version.h
        @ONLY
)

add_library(ion
        socket_fd.h
        tls_conn.cpp
        tls_conn.h
        tcp_listener.h
        tcp_listener.cpp
        http2_frames.h
        http2_conn.cpp
        http2_conn.h
        http2_conn.cpp
        http2_server.cpp
        http2_server.h
        router.cpp
        router.h
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
        headers/header_block_encoder.cpp
        headers/header_block_encoder.h
        server_config.h
        server_config.cpp
        static_file_handler.cpp
        static_file_handler.h
        file_reader.cpp
        file_reader.h
        http2_frame_reader.cpp
        http2_frame_reader.h
        frame_error.h
        headers/header_field.h
)

target_compile_options(ion PRIVATE
        -ferror-limit=0
        -ftemplate-backtrace-limit=0
        -fdiagnostics-show-template-tree
        -fdiagnostics-show-note-include-stack
)

target_include_directories(ion PUBLIC
        .
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(ion PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
)

target_compile_features(ion PUBLIC cxx_std_23)
