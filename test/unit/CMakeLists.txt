FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit-test
        test_frame_parsing.cpp
        headers/test_hb_decoder.cpp
        headers/test_huffman_tree.cpp
        headers/test_bit_reader.cpp
        headers/test_hb_encoder.cpp
        headers/test_dynamic_table.cpp
)

target_link_libraries(unit-test
        PRIVATE
        ion
        Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(unit-test)
