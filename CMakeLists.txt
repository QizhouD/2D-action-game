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

set(GAME_SOURCES
    ${GAME_ROOT}/main.cpp
    ${GAME_ROOT}/source/Components/InputComponent.cpp
    ${GAME_ROOT}/source/Components/PlayerStateComponent.cpp
    ${GAME_ROOT}/source/Components/VelocityComponent.cpp
    ${GAME_ROOT}/source/core/AudioManager.cpp
    ${GAME_ROOT}/source/core/Board.cpp
    ${GAME_ROOT}/source/core/Game.cpp
    ${GAME_ROOT}/source/core/GameCommand.cpp
    ${GAME_ROOT}/source/core/InputHandler.cpp
    ${GAME_ROOT}/source/core/Tile.cpp
    ${GAME_ROOT}/source/entities/Enemy.cpp
    ${GAME_ROOT}/source/entities/Entity.cpp
    ${GAME_ROOT}/source/entities/Fire.cpp
    ${GAME_ROOT}/source/entities/Player.cpp
    ${GAME_ROOT}/source/graphics/AnimBase.cpp
    ${GAME_ROOT}/source/graphics/AnimDirectional.cpp
    ${GAME_ROOT}/source/graphics/Hud.cpp
    ${GAME_ROOT}/source/graphics/SpriteSheet.cpp
    ${GAME_ROOT}/source/graphics/SpriteSheetGraphicsComponent.cpp
    ${GAME_ROOT}/source/graphics/Window.cpp
    ${GAME_ROOT}/source/systems/AISystem.cpp
    ${GAME_ROOT}/source/systems/ColliderSystem.cpp
    ${GAME_ROOT}/source/systems/GraphicsSystem.cpp
    ${GAME_ROOT}/source/systems/InputSystem.cpp
    ${GAME_ROOT}/source/systems/MovementSystem.cpp
    ${GAME_ROOT}/source/systems/PrintDebugSystem.cpp
    ${GAME_ROOT}/source/systems/TTLSystem.cpp
    ${GAME_ROOT}/source/utils/Rectangle.cpp
)

add_executable(MiniGame ${GAME_SOURCES})
target_include_directories(MiniGame PRIVATE ${GAME_ROOT})
target_link_libraries(MiniGame PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)

if(MSVC)
    target_compile_options(MiniGame PRIVATE /W3 /utf-8)
else()
    target_compile_options(MiniGame PRIVATE -Wall -Wextra)
    if(MINGW)
        # Avoid shipping libstdc++/libgcc DLLs next to the exe.
        target_link_options(MiniGame PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()

# ---------------------------------------------------------------------------
# Runtime assets: copy next to the executable so it can be launched directly.
# ---------------------------------------------------------------------------
set(GAME_ASSET_DIRS audio font img levels)
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
