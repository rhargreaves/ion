FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

find_package(CURL REQUIRED)

add_executable(int-test
        test_server.cpp
)

target_link_libraries(int-test
        PRIVATE
        ion
        Catch2::Catch2WithMain
        CURL::libcurl
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(int-test)
