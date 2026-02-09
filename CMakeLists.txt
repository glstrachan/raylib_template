cmake_minimum_required(VERSION 3.10)
project(bigmode_jam_26)

# set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_C_STANDARD 11)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GRAPHICS "GRAPHICS_API_OPENGL_43")
add_subdirectory(external/raylib)

set(SOURCES
    src/foomain.c
    src/main.c
    src/dialog.c
    src/characters.c
    src/jump.c
    src/encounter.c
    src/pick_items.c
    src/font_manager.c
    external/clay/clay_renderer_raylib.c

    src/memory_game.c
    src/smile_game.c
    src/shotgun_game.c
    src/rhythm_game.c
)

include_directories(
    src
)

add_executable(${PROJECT_NAME} WIN32  ${SOURCES})
target_link_libraries(${PROJECT_NAME} raylib)
target_compile_options(raylib PRIVATE -DSUPPORT_HIGH_DPI)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fcolor-diagnostics -fansi-escape-codes -Wno-initializer-overrides -DSUPPORT_HIGH_DPI)

# add_compile_definitions(OC_ENABLE_ASAN_MALLOC)
# add_compile_options(-fno-omit-frame-pointer -fsanitize=address -static-libasan)
# add_link_options(-fno-omit-frame-pointer -fsanitize=address -static-libasan   -O1 -g -fsanitize=address -fno-omit-frame-pointer )
# add_link_options(--subsystem,windows)
