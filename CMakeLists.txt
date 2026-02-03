cmake_minimum_required(VERSION 3.10)
project(raylib_template)

set(CMAKE_C_COMPILER "C:/Program Files/LLVM/bin/clang.exe")
set(CMAKE_C_STANDARD 11)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GRAPHICS "GRAPHICS_API_OPENGL_43")
add_subdirectory(external/raylib)

set(SOURCES
    src/main.c
    src/dialog.c
    src/characters.c
    src/jump.c
    src/encounter.c
    src/pick_items.c

    src/memory_game.c
    src/smile_game.c
)

include_directories(
    src
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} raylib)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -fcolor-diagnostics -fansi-escape-codes -Wno-initializer-overrides)