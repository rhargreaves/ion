add_executable(ion_wasm
        main.cpp
        log_sink.h
        wasm_transport.h
        server.cpp
        server.h
        wasm_transport.cpp
        test_routes.cpp
        test_routes.h
)
target_link_libraries(ion_wasm PRIVATE ion)

set_target_properties(ion_wasm PROPERTIES
        OUTPUT_NAME ion
        SUFFIX ".wasm"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_link_options(ion_wasm PRIVATE
        -sSTANDALONE_WASM
        -sALLOW_MEMORY_GROWTH=1
        -Wl,--no-entry
        -sERROR_ON_UNDEFINED_SYMBOLS=1
        -sEXPORTED_FUNCTIONS=_malloc,_free)
