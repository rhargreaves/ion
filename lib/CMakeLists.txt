configure_file(
        ${CMAKE_SOURCE_DIR}/lib/version.h.in
        ${CMAKE_BINARY_DIR}/generated/version.h
        @ONLY
)

set(ION_CORE_SOURCES
        http2_frames.h
        http2_conn.cpp
        http2_conn.h
        http2_conn.cpp
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
        hpack/header_field.h
        hpack/int_decoder.cpp
        hpack/int_decoder.h
        hpack/int_encoder.cpp
        hpack/int_encoder.h
        http2_frame_reader.cpp
        http2_frame_reader.h
        frame_error.h
        system_clock.cpp
        system_clock.h
        access_log.cpp
        access_log.h
        stop_reason.cpp
        stop_reason.h
        transports/transport.h
        hpack/byte_reader.cpp
        file_store.h
)

set(ION_NATIVE_SOURCES
        socket_fd.h
        transports/tls_transport.cpp
        transports/tls_transport.h
        tcp_listener.h
        tcp_listener.cpp
        http2_server.cpp
        http2_server.h
        server_config.h
        server_config.cpp
        static_file_handler.cpp
        static_file_handler.h
        file_reader.cpp
        file_reader.h
        transports/tcp_transport.cpp
        socket_fd.cpp
        pollers/poller.h
        pollers/poll_poller.cpp
        pollers/poll_poller.h
        pollers/poller.cpp
        pollers/epoll_poller.cpp
        pollers/epoll_poller.h
        transports/tls_context.cpp
        transports/tls_context.h
        native_file_store.cpp
        native_file_store.h
)

set(ION_WASM_SOURCES
)

if (ION_WASM)
    add_library(ion ${ION_CORE_SOURCES} ${ION_WASM_SOURCES})
    target_link_libraries(ion PUBLIC
            spdlog::spdlog_header_only
    )
    target_compile_definitions(ion PUBLIC
            ION_WASM=1
            ION_ENABLE_OTEL=0
    )

else ()
    add_library(ion ${ION_CORE_SOURCES} ${ION_NATIVE_SOURCES})
    target_link_libraries(ion PUBLIC
            OpenSSL::SSL
            OpenSSL::Crypto
            spdlog::spdlog
            opentelemetry-cpp::api
    )
endif ()

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

target_compile_features(ion PUBLIC cxx_std_23)
