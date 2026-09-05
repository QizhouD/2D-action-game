#include "../../include/graphics/Window.h"
#include "../../include/core/Game.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

Window::Window()
    : logicalSize({ 0, 0 })
    , pixelSize({ 0, 0 })
    , windowTitle("")
    , isDone(false)
    , isFullscreen(false)
    , focused(true)
    , debugDraw(false)
{
}

Window::~Window() { destroy(); }

void Window::loadFont(const std::string& fontFile)
{
    if (!guiFont.loadFromFile(fontFile)) {
        throw std::runtime_error("Font file not found for Window: " + fontFile);
    }

    fpsText.setCharacterSize(fontSize);
    fpsText.setFillColor(sf::Color::Red);
    fpsText.setFont(guiFont);
    fpsText.setPosition(10.f, 10.f);
    fpsText.setString("FPS: --");

    pausedText.setCharacterSize(fontSize + 10);
    pausedText.setFillColor(sf::Color::Blue);
    pausedText.setFont(guiFont);
    pausedText.setString("PAUSED!");

    healthText.setCharacterSize(fontSize);
    healthText.setFillColor(sf::Color::Green);
    healthText.setFont(guiFont);
    healthText.setPosition(10.f, 60.f);
    healthText.setString("Health: 100/100");
}

void Window::setup(const std::string& title, const sf::Vector2u& size)
{
    windowTitle = title;
    logicalSize = size;
    create();
}

void Window::create()
{
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    if (isFullscreen) {
        pixelSize = { desktop.width, desktop.height };
        window.create(desktop, windowTitle, sf::Style::Fullscreen);
    }
    else {
        // Leave room for the title bar / task bar; never upscale a small map.
        const float margin = 0.9f;
        const float sx = (desktop.width * margin) / static_cast<float>(logicalSize.x);
        const float sy = (desktop.height * margin) / static_cast<float>(logicalSize.y);
        const float scale = std::min(1.f, std::min(sx, sy));
        pixelSize = {
            static_cast<unsigned>(logicalSize.x * scale),
            static_cast<unsigned>(logicalSize.y * scale)
        };
        window.create({ pixelSize.x, pixelSize.y, 32 }, windowTitle, sf::Style::Default);
    }

    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    applyView();

    // Centre the pause banner in logical space.
    const auto b = pausedText.getLocalBounds();
    pausedText.setOrigin(b.left + b.width * 0.5f, b.top);
    pausedText.setPosition(logicalSize.x * 0.5f, 10.f);
}

void Window::applyView()
{
    if (logicalSize.x == 0 || logicalSize.y == 0) return;

    sf::View view(sf::FloatRect(0.f, 0.f,
        static_cast<float>(logicalSize.x), static_cast<float>(logicalSize.y)));

    const float winRatio = static_cast<float>(pixelSize.x) / static_cast<float>(pixelSize.y);
    const float viewRatio = static_cast<float>(logicalSize.x) / static_cast<float>(logicalSize.y);

    if (winRatio > viewRatio) {          // window is wider: pillar-box
        const float w = viewRatio / winRatio;
        view.setViewport({ (1.f - w) * 0.5f, 0.f, w, 1.f });
    }
    else {                               // window is taller: letter-box
        const float h = winRatio / viewRatio;
        view.setViewport({ 0.f, (1.f - h) * 0.5f, 1.f, h });
    }
    window.setView(view);
}

void Window::destroy()
{
    window.close();
}

void Window::pollEvents()
{
    keysPressed.clear();

    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Closed:
            isDone = true;
            break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::F5)
                toggleFullscreen();
            else
                keysPressed.push_back(event.key.code);
            break;
        case sf::Event::Resized:
            pixelSize = { event.size.width, event.size.height };
            applyView();
            break;
        case sf::Event::LostFocus:
            focused = false;
            break;
        case sf::Event::GainedFocus:
            focused = true;
            break;
        default:
            break;
        }
    }
}

bool Window::wasKeyPressed(sf::Keyboard::Key key) const
{
    return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
}

void Window::toggleFullscreen()
{
    isFullscreen = !isFullscreen;
    redraw();
}

void Window::redraw()
{
    destroy();
    create();
}

void Window::beginDraw() { window.clear(sf::Color::Black); }
void Window::endDraw() { window.display(); }

bool Window::isWindowDone() const { return isDone; }
bool Window::isWindowFullscreen() const { return isFullscreen; }

void Window::setFPS(int fps)
{
    fpsText.setString("FPS: " + std::to_string(fps));
}

void Window::drawGUI(const Game& game)
{
    window.draw(fpsText);
    if (game.getPlayer()) {
        auto playerHealth = game.getPlayer()->getHealthComp()->getHealth();
        auto maxHealth = game.getPlayer()->getHealthComp()->getMaxHealth();
        std::ostringstream ss;
        ss << "Health: " << playerHealth << "/" << maxHealth;
        healthText.setString(ss.str());
        window.draw(healthText);
    }
    if (game.isPaused()) {
        window.draw(pausedText);
    }
}

void Window::draw(const sf::Drawable& drawable) {
    window.draw(drawable);
}
