add_executable(ion-server main.cpp
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
)

target_link_libraries(ion-server PRIVATE
        OpenSSL::SSL OpenSSL::Crypto
        spdlog::spdlog
        CLI11::CLI11
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
