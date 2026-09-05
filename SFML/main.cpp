#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "include/core/Game.h"

namespace {

std::vector<std::string> readLevelFile(const std::string& path)
{
    std::ifstream in{ path };
    if (!in) {
        throw std::runtime_error("Level file not found: " + path);
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    try {
        Game game;
        game.init(readLevelFile("levels/lvl0.txt"));

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
