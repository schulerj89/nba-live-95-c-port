cmake_minimum_required(VERSION 3.16)

project(nba_live_95_c_port LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

set(NBA95_SOURCES
    src/main.c
    src/win32_game_main.c
    src/nba_rom.c
    src/nba_font.c
    src/nba_renderer.c
    src/nba_game.c
)

add_executable(nba95_port ${NBA95_SOURCES})

if(MSVC)
    target_compile_options(nba95_port PRIVATE /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS)
endif()

if(WIN32)
    target_link_libraries(nba95_port PRIVATE user32 gdi32 winmm)
endif()
