configure_file(
        ${CMAKE_SOURCE_DIR}/lib/version.h.in
        ${CMAKE_BINARY_DIR}/generated/version.h
        @ONLY
)

add_library(ion
        socket_fd.h
        transports/tls_transport.cpp
        transports/tls_transport.h
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
        hpack/header_block_decoder.cpp
        hpack/header_block_decoder.h
        hpack/huffman_tree.cpp
        hpack/huffman_tree.h
        hpack/huffman_codes.h
        hpack/header_static_table.h
        hpack/byte_reader.h
        hpack/http_header.h
        hpack/static_http_header.h
        hpack/dynamic_table.cpp
        hpack/dynamic_table.h
        hpack/header_block_encoder.cpp
        hpack/header_block_encoder.h
        server_config.h
        server_config.cpp
        static_file_handler.cpp
        static_file_handler.h
        file_reader.cpp
        file_reader.h
        http2_frame_reader.cpp
        http2_frame_reader.h
        frame_error.h
        hpack/header_field.h
        hpack/int_decoder.cpp
        hpack/int_decoder.h
        system_clock.cpp
        system_clock.h
        access_log.cpp
        access_log.h
        stop_reason.cpp
        stop_reason.h
        transports/transport.h
        transports/tcp_transport.cpp
        socket_fd.cpp
        hpack/byte_reader.cpp
)

target_compile_options(ion PUBLIC ${ION_DEV_FLAGS})
target_link_options(ion PUBLIC ${ION_DEV_FLAGS})

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fstack-protector-strong -fPIC)
    add_definitions(-D_FORTIFY_SOURCE=2)
endif ()

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
