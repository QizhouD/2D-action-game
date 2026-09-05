[![CI](https://github.com/QizhouD/2D-action-game/actions/workflows/ci.yml/badge.svg)](https://github.com/QizhouD/2D-action-game/actions/workflows/ci.yml)

A small top-down action game in C++17 / SFML 2.6: a dwarf clears mushroom-infested mazes with an axe and wood-fuelled fireballs. Built as a showcase of a lightweight entity/component/system design plus the Command, Observer and Service Locator patterns.

- Fixed-timestep simulation (60 Hz) with variable-rate rendering and a following camera
- Component-based entities (`Position`, `Velocity`, `Input`, `Health`, `TTL`, `AI`) driven by a "big array" system pipeline: input → AI → movement → TTL
- Tile-map collision, axe hitboxes, projectile TTL, enemy patrol/chase AI, boss, potion drops
- Full game flow: menu with level select, pause, level clear, game over, victory, persistent high score / unlocked levels
- HUD with toasts, damage flash, CPU particle effects, pooled audio with synthesized fallbacks, gamepad support
- All gameplay tuning in `config/balance.ini`; levels are plain text maps; textures are shared through a `ResourceManager`
- Scripted input + screenshot mode for headless regression runs, and SFML-free unit tests for the parsers

## Build & Run

Two build paths are supported:

### CMake (MinGW-w64 or MSVC) — recommended

Requirements: CMake ≥ 3.16, Ninja, a C++17 compiler, and an SFML 2.5+/2.6 package that matches the compiler.
On Windows without Visual Studio the tested combination is **WinLibs GCC 13.1.0 (MSVCRT, posix-seh)** + **SFML 2.6.1 "GCC 13.1.0 MinGW 64-bit"**, unpacked side by side:

```
<toolchain>/mingw64/          # from https://github.com/brechtsanders/winlibs_mingw/releases
<toolchain>/SFML-2.6.1/       # from https://github.com/SFML/SFML/releases/tag/2.6.1
```

```powershell
pip install cmake ninja          # if not already available
.\build.ps1 -Run                 # configure + build + launch (toolchain dir defaults to ..\toolchain)
.\build.ps1 -Test                # configure + build + run the unit tests
.\build.ps1 -ToolchainDir D:\my\toolchain -Config Debug
```

Or by hand:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSFML_DIR=<toolchain>/SFML-2.6.1/lib/cmake/SFML
cmake --build build
ctest --test-dir build --output-on-failure
cmake --build build --target run
```

On Linux, `apt install libsfml-dev` and configure with `-DMINIGAME_SFML_STATIC=OFF` (see `.github/workflows/ci.yml`).

The executable is linked statically; assets (`audio/ config/ font/ img/ levels/`) and `openal32.dll` are copied next to `build/MiniGame.exe`. The game locates its assets relative to the executable, so it can be started from any working directory (double-click, shortcut, CI).

### Tests

`tests/test_main.cpp` covers the SFML-free core (`LevelParser`, `Balance`, `SaveData`): parser edge cases, error reporting, and a check that every shipped level is well-formed and that `config/balance.ini` matches the compiled defaults. Run with `ctest --test-dir build` or `.\build.ps1 -Test`. CI builds Windows (MinGW, static SFML) and Linux (system SFML) on every push.

### Visual Studio 2022 (`SFML/SFML.sln`)

The original project expects SFML 2.5.1 (static, x64) at `D:\SFML\SFML-2.5.1`. Adjust the include/library paths in `SFML.vcxproj` if your install differs, and run with the working directory set to `SFML/`.

### Controls

| Key | Action |
| --- | --- |
| `W A S D` / arrow keys / gamepad stick | Move (`Tab` toggles between the two keyboard schemes) |
| `Space` / gamepad `A` | Axe attack — chop logs to collect wood, hit enemies |
| `Left Shift` / gamepad `B` | Shout — spend 1 wood to launch a fireball in the direction you last walked |
| `Enter` / gamepad `A` | Confirm in menus (start / next level); `Left`/`Right` pick an unlocked start level |
| `Esc` / gamepad `Start` | Pause / resume (`R` restart level, `Q` back to menu while paused) |
| `M` | Mute / unmute |
| `F1` | Toggle debug bounding boxes and FPS |
| `F5` | Toggle fullscreen |

Goal: kill every mushroom on the level, then step on the golden exit circle. Four levels; the last one has a boss. Levels bigger than the window scroll with the camera. Best score and unlocked levels are stored in `save.txt` next to the executable. The game pauses itself when the window loses focus.

### Levels

`levels/levels.txt` lists the level files in play order; the tile legend is documented at the top of that file. Add a new `.txt` map and a line in the list to add a level — no code changes needed. Maps must contain exactly one `*` start; ragged rows are padded with wall, and unknown characters are reported with row/column.

### Tuning

`config/balance.ini` holds every gameplay number (player speed/HP/axe, fireball speed/lifetime/damage, pickup amounts, per-enemy stats for `mushroom` and `boss`). Every key is optional and falls back to the defaults in `include/core/Balance.h`; unknown keys or bad values are printed at startup and skipped.

### Command line

| Flag | Meaning |
| --- | --- |
| `--levels <list.txt>` | Use another level list (level files are resolved relative to the list) |
| `--script <file>` | Replace the keyboard with a timed script and dump frames to PNG — works without window focus, handy for regression checks (format documented in `SFML/include/graphics/Window.h`) |
