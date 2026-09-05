cmake_minimum_required(VERSION 3.16)
project(MiniGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# SFML
#   Point CMake at an SFML install with -DSFML_DIR=<sfml>/lib/cmake/SFML
#   (or -DCMAKE_PREFIX_PATH=<sfml>). Static linking is used so the produced
#   executable is self-contained apart from openal32.dll (audio module).
# ---------------------------------------------------------------------------
option(MINIGAME_SFML_STATIC "Link SFML statically" ON)
if(MINIGAME_SFML_STATIC)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system audio)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(GAME_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/SFML)

# Pure C++ logic with no SFML dependency: shared by the game and the tests.
add_library(minigame_core STATIC
    ${GAME_ROOT}/source/core/Balance.cpp
    ${GAME_ROOT}/source/core/LevelParser.cpp
    ${GAME_ROOT}/source/core/SaveData.cpp
)
target_include_directories(minigame_core PUBLIC ${GAME_ROOT})

set(GAME_SOURCES
    ${GAME_ROOT}/main.cpp
    ${GAME_ROOT}/source/Components/InputComponent.cpp
    ${GAME_ROOT}/source/Components/VelocityComponent.cpp
    ${GAME_ROOT}/source/core/AudioManager.cpp
    ${GAME_ROOT}/source/core/Board.cpp
    ${GAME_ROOT}/source/core/Game.cpp
    ${GAME_ROOT}/source/core/GameCommand.cpp
    ${GAME_ROOT}/source/core/InputHandler.cpp
    ${GAME_ROOT}/source/core/ResourceManager.cpp
    ${GAME_ROOT}/source/core/Tile.cpp
    ${GAME_ROOT}/source/entities/Enemy.cpp
    ${GAME_ROOT}/source/entities/Entity.cpp
    ${GAME_ROOT}/source/entities/Fire.cpp
    ${GAME_ROOT}/source/entities/Player.cpp
    ${GAME_ROOT}/source/graphics/AnimBase.cpp
    ${GAME_ROOT}/source/graphics/AnimDirectional.cpp
    ${GAME_ROOT}/source/graphics/Hud.cpp
    ${GAME_ROOT}/source/graphics/ParticleSystem.cpp
    ${GAME_ROOT}/source/graphics/SpriteSheet.cpp
    ${GAME_ROOT}/source/graphics/Window.cpp
    ${GAME_ROOT}/source/systems/AISystem.cpp
    ${GAME_ROOT}/source/systems/InputSystem.cpp
    ${GAME_ROOT}/source/systems/MovementSystem.cpp
    ${GAME_ROOT}/source/systems/TTLSystem.cpp
    ${GAME_ROOT}/source/utils/Rectangle.cpp
)

add_executable(MiniGame ${GAME_SOURCES})
target_include_directories(MiniGame PRIVATE ${GAME_ROOT})
target_link_libraries(MiniGame PRIVATE minigame_core sfml-graphics sfml-window sfml-system sfml-audio)

# ---------------------------------------------------------------------------
# Unit tests (no SFML, no window): ctest --test-dir build
# ---------------------------------------------------------------------------
option(MINIGAME_BUILD_TESTS "Build unit tests" ON)
if(MINIGAME_BUILD_TESTS)
    enable_testing()
    add_executable(minigame_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp)
    target_link_libraries(minigame_tests PRIVATE minigame_core)
    target_compile_definitions(minigame_tests PRIVATE
        MINIGAME_LEVELS_DIR="${GAME_ROOT}/levels"
        MINIGAME_CONFIG_DIR="${GAME_ROOT}/config")
    add_test(NAME unit COMMAND minigame_tests)
endif()

foreach(tgt minigame_core MiniGame)
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /W3 /utf-8)
    else()
        target_compile_options(${tgt} PRIVATE -Wall -Wextra)
    endif()
endforeach()
if(MINGW)
    # Avoid shipping libstdc++/libgcc DLLs next to the exe.
    target_link_options(MiniGame PRIVATE -static-libgcc -static-libstdc++ -static)
    if(TARGET minigame_tests)
        target_link_options(minigame_tests PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()

# ---------------------------------------------------------------------------
# Runtime assets: copy next to the executable so it can be launched directly.
# ---------------------------------------------------------------------------
set(GAME_ASSET_DIRS audio config font img levels)
foreach(dir ${GAME_ASSET_DIRS})
    add_custom_command(TARGET MiniGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${GAME_ROOT}/${dir} $<TARGET_FILE_DIR:MiniGame>/${dir}
        COMMENT "Copying ${dir}/ next to MiniGame")
endforeach()

# openal32.dll is the only SFML dependency that cannot be linked statically.
if(WIN32 AND SFML_DIR)
    get_filename_component(_sfml_root "${SFML_DIR}/../../.." ABSOLUTE)
    if(EXISTS "${_sfml_root}/bin/openal32.dll")
        add_custom_command(TARGET MiniGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_sfml_root}/bin/openal32.dll" $<TARGET_FILE_DIR:MiniGame>
            COMMENT "Copying openal32.dll next to MiniGame")
    endif()
endif()

# `cmake --build build --target run` launches the game from its output dir.
add_custom_target(run
    COMMAND MiniGame
    WORKING_DIRECTORY $<TARGET_FILE_DIR:MiniGame>
    USES_TERMINAL)
