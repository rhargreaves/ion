find_package(Catch2 CONFIG REQUIRED)

set(ION_TEST_COMMON
        test_frame_parsing.cpp
        hpack/test_hb_decoder.cpp
        hpack/test_huffman_tree.cpp
        hpack/test_bit_reader.cpp
        hpack/test_hb_encoder.cpp
        hpack/test_dynamic_table.cpp
        hpack/test_int_decoder.cpp
        hpack/test_byte_reader.cpp
        hpack/test_int_encoder.cpp
)

set(ION_TEST_NATIVE
        test_router_middleware.cpp  # todo: fix dep on NativeFileStore
        test_router.cpp
        test_file_reader.cpp
)

set(ION_TEST_WASM)

if (ION_WASM)
    add_executable(unit-test ${ION_TEST_COMMON} ${ION_TEST_WASM})
else ()
    add_executable(unit-test ${ION_TEST_COMMON} ${ION_TEST_NATIVE})
endif ()

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
