add_executable(ion-server
        main.cpp
        args.cpp
        signal_handler.cpp
        signal_handler.h
        proc_ctrl.cpp
        proc_ctrl.h
        test_routes.cpp
        test_routes.h
)

target_link_libraries(ion-server PRIVATE
        ion
        CLI11::CLI11
)

target_include_directories(ion-server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fstack-protector-strong -fPIC)
    add_definitions(-D_FORTIFY_SOURCE=2)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ion-server PRIVATE
            -fsanitize=address
            -fno-omit-frame-pointer
            -g
    )
    target_link_options(ion-server PRIVATE
            -fsanitize=address
    )
else ()
    target_compile_options(ion-server PRIVATE -fPIE)
    if (NOT APPLE)
        target_link_options(ion-server PRIVATE
                -pie
                "LINKER:-z,relro"
                "LINKER:-z,now"
        )
    endif ()
endif ()

