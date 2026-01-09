# 2D Action Game - ECS Engine

A complete 2D action game built with a flexible ECS (Entity Component System) architecture, demonstrating modern C++ design patterns and performance-oriented engine design.

## Features

- **Flexible ECS Architecture:** Support for multiple implementations including Big Array, Archetypes, and Packed Arrays.
- **Design Patterns:** Implements Command, Observer, and Service Locator patterns for modularity.
- **Adaptive Game Loop:** Target 60 FPS rendering with smooth updates.
- **Dual Input Modes:** Support for WASD and Arrow keys with real-time toggle functionality.
- **Collision System:** Entity collision detection with type-specific callback handling.
- **Audio Management:** Managed via Service Locator for global accessibility.
- **Runtime Entity Management:** Spawning, destruction, and TTL (Time-To-Live) handling.
- **Level Loading:** Loads levels from text-based map files (`SFML/levels/`).
- **Real-time Monitoring:** FPS counter and GUI status display.
- **Component System:** Support for Players, Pickups (Potions, Logs), and Projectiles (Fire).

## Project Specifications

- **Language:** C++14
- **Toolchain:** Visual Studio 2022 (v143)
- **Library:** SFML 2.5.1
- **Platform:** x64
- **Linking:** Static (`SFML_STATIC`)

## Prerequisites

- **Visual Studio 2022** with the "Desktop development with C++" workload.
- **SFML 2.5.1:** The project is pre-configured to look for SFML at `D:\SFML\SFML-2.5.1\`.
- **Links**: https://aka.ms/vs/17/release/vs_community.exe （Visual Studio 2022) /https://www.sfml-dev.org/download/sfml/2.5.1/ (SFML)
## Setup & Configuration

If your SFML installation is in a different location, follow these steps:

1. Open **`SFML/SFML.sln`** in Visual Studio.
2. Right-click the **SFML** project in Solution Explorer and select **Properties**.
3. **Include Paths:** Go to `C/C++ -> General -> Additional Include Directories` and update to your SFML `include` folder.
4. **Library Paths:** Go to `Linker -> General -> Additional Library Directories` and update to your SFML `lib` folder.

## How to Run

1. Open **`SFML/SFML.sln`**.
2. Set the configuration to **Debug** or **Release**.
3. Set the platform to **x64**.
4. Press **F5** (Local Windows Debugger) to build and run.

## Controls & Gameplay

- **Movement:** WASD or Arrow keys.
- **Input Toggle:** Switch between movement modes (WASD vs Arrows).
- **Objective:** Navigate levels, collect items, and interact with the environment.
- **Assets:** Assets are loaded from `SFML/img/`, `SFML/audio/`, and `SFML/font/`.
