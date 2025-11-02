FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-tests
        test_frame_parsing.cpp
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

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(unit-tests)
