#include <SFML/Graphics.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include "include/core/Game.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

// Directory containing the running executable (falls back to argv[0]).
static fs::path executableDir(const char* argv0)
{
    std::error_code ec;
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return fs::path(buf).parent_path();
#elif defined(__linux__)
    const fs::path self = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return self.parent_path();
#endif
    const fs::path p = fs::absolute(argv0 ? argv0 : ".", ec);
    return ec ? fs::current_path() : p.parent_path();
}

// Assets are addressed with paths like "img/log.png". Running from the source
// tree (cwd = SFML/) already works; when launched from anywhere else (double
// click, shortcut, CI) switch to the exe's directory, where the build copies
// the asset folders.
static void chdirToAssets(const char* argv0)
{
    std::error_code ec;
    if (fs::exists("levels/levels.txt", ec)) return;
    const fs::path dir = executableDir(argv0);
    if (fs::exists(dir / "levels" / "levels.txt", ec))
        fs::current_path(dir, ec);
}

int main(int argc, char** argv)
{
    try {
        // --levels <list.txt>: alternative level list (level files are resolved
        //                      relative to the list's directory)
        // --script <file>:     drive the game from a key/screenshot script (see Window.h)
        std::string levelList;
        std::string script;
        for (int i = 1; i + 1 < argc; ++i) {
            const std::string arg = argv[i];
            // User paths are relative to the *invocation* cwd: pin them before chdir.
            if (arg == "--levels")      levelList = fs::absolute(argv[++i]).string();
            else if (arg == "--script") script = fs::absolute(argv[++i]).string();
        }

        chdirToAssets(argc > 0 ? argv[0] : nullptr);
        if (levelList.empty()) levelList = "levels/levels.txt";

        Game game;
        game.init(levelList);
        if (!script.empty()) game.getWindow()->loadScript(script);

        // Fixed-timestep simulation (60 Hz) with variable-rate rendering.
        const float dt = 1.f / 60.f;
        const float maxFrame = 0.25f;   // clamp after stalls (window drag, breakpoint...)
        float accumulator = 0.f;
        float fpsTimer = 0.f;
        int fpsFrames = 0;

        sf::Clock clock;
        while (!game.getWindow()->isWindowDone()) {
            float frame = clock.restart().asSeconds();
            if (frame > maxFrame) frame = maxFrame;
            accumulator += frame;

            game.handleInput();
            while (accumulator >= dt) {
                game.update(dt);
                accumulator -= dt;
            }
            game.render(frame);

            fpsTimer += frame;
            ++fpsFrames;
            if (fpsTimer >= 0.5f) {
                game.setFPS(static_cast<int>(fpsFrames / fpsTimer + 0.5f));
                fpsTimer = 0.f;
                fpsFrames = 0;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
