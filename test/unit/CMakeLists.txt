FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-test
        test_frame_parsing.cpp
        hpack/test_hb_decoder.cpp
        hpack/test_huffman_tree.cpp
        hpack/test_bit_reader.cpp
        hpack/test_hb_encoder.cpp
        hpack/test_dynamic_table.cpp
        test_router.cpp
        test_file_reader.cpp
        hpack/test_int_decoder.cpp
        test_router_middleware.cpp
        hpack/test_byte_reader.cpp
)

target_link_libraries(unit-test
        PRIVATE
        ion
        Catch2::Catch2WithMain
)

target_compile_options(unit-test PRIVATE ${ION_DEV_FLAGS})
target_link_options(unit-test PRIVATE ${ION_DEV_FLAGS})

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(unit-test)
