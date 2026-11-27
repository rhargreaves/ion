#add_library(ion_wasm_platform STATIC
#)
#target_link_libraries(ion_wasm_platform PUBLIC ion)

add_executable(ion_wasm
        ion_wasm.cpp
        ion_log_sink.h
        wasm_transport.h
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
        -sEXPORTED_FUNCTIONS=_ion_init
        -sERROR_ON_UNDEFINED_SYMBOLS=0)
