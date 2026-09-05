This project demonstrates a complete 2D game built with flexible ECS architecture options, implementing design patterns such as Command, Observer, and Service Locator. Key features include:
Adaptive game loop with target 60FPS rendering
Dual input mode support (WASD/Arrow keys) with toggle functionality
Entity collision detection with type-specific callback handling
Audio management via Service Locator pattern
Runtime entity spawning, destruction, and TTL (Time-To-Live) handling
Level loading from text-based map files
Real-time FPS monitoring and GUI status display
Component-based entity system supporting players, pickups, and projectiles
The engine architecture allows seamless switching between ECS implementations (Big Array, Archetypes, Packed Arrays) to compare performance characteristics while maintaining consistent gameplay functionality. Core systems include input processing, movement, collision detection, gameplay logic, and rendering, all designed for modularity and extensibility

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
.\build.ps1 -ToolchainDir D:\my\toolchain -Config Debug
```

Or by hand:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSFML_DIR=<toolchain>/SFML-2.6.1/lib/cmake/SFML
cmake --build build
cmake --build build --target run
```

The executable is linked statically; assets (`audio/ font/ img/ levels/`) and `openal32.dll` are copied next to `build/MiniGame.exe`, so it can be launched directly from that folder.

### Visual Studio 2022 (`SFML/SFML.sln`)

The original project expects SFML 2.5.1 (static, x64) at `D:\SFML\SFML-2.5.1`. Adjust the include/library paths in `SFML.vcxproj` if your install differs, and run with the working directory set to `SFML/`.

### Controls

| Key | Action |
| --- | --- |
| `W A S D` / arrow keys | Move (`Enter` toggles between the two schemes) |
| `Space` | Axe attack — chop logs to collect wood |
| `Left Shift` | Shout — spend 1 wood to launch a fireball |
| `Esc` | Pause |
| `F5` | Toggle fullscreen |
