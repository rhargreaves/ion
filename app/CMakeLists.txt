add_executable(ion-server
        main.cpp
        args.cpp
)

target_link_libraries(ion-server PRIVATE
        ion
        CLI11::CLI11
)

target_include_directories(ion-server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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

