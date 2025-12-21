FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

find_package(CURL REQUIRED)

add_executable(int-test
        test_server.cpp
        curl_client.h
        wait_for_port.h
        test_server_runner.h
        test_static.cpp
        wait_for_port.cpp
        test_helpers.h
        test_helpers.cpp
)

target_link_libraries(int-test
        PRIVATE
        ion
        spdlog
        Catch2::Catch2WithMain
        CURL::libcurl
)

target_compile_definitions(int-test
        PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
        PROJECT_BINARY_DIR="${CMAKE_BINARY_DIR}")

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(int-test)
