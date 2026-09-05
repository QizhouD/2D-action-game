#include <SFML/Graphics.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include "include/core/Game.h"

int main(int argc, char** argv)
{
    try {
        // --levels <list.txt>: alternative level list (level files are resolved
        //                      relative to the list's directory)
        // --script <file>:     drive the game from a key/screenshot script (see Window.h)
        std::string levelList = "levels/levels.txt";
        std::string script;
        for (int i = 1; i + 1 < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--levels")      levelList = argv[++i];
            else if (arg == "--script") script = argv[++i];
        }

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
