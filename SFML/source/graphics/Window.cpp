#include "../../include/graphics/Window.h"
#include "../../include/core/Game.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace {
static void applyLetterboxToView(sf::View& view, unsigned int windowWidth, unsigned int windowHeight)
{
    if (windowHeight == 0 || windowWidth == 0) return;
    float windowRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float viewRatio   = view.getSize().x / view.getSize().y;

    float sizeX = 1.f, sizeY = 1.f;
    float posX  = 0.f, posY  = 0.f;

    if (windowRatio > viewRatio) {
        sizeX = viewRatio / windowRatio;
        posX  = (1.f - sizeX) * 0.5f;
    } else if (windowRatio < viewRatio) {
        sizeY = windowRatio / viewRatio;
        posY  = (1.f - sizeY) * 0.5f;
    }
    view.setViewport({ posX, posY, sizeX, sizeY });
}
}

Window::Window()
    : windowTitle("")
    , windowSize({ 0, 0 })
    , isFullscreen(false)
    , isDone(false)
{
}

Window::~Window() { destroy(); }

void Window::loadFont(const std::string& fontFile)
{
    if (!guiFont.loadFromFile(fontFile)) {
        throw std::runtime_error("Font file not found for Window: " + fontFile);
    }

    // Load potion icon for health segments UI (fallback to rectangle bar if missing)
    potionIconTexture.loadFromFile("img/potion.png");

    // Set up FPS text.
    fpsText.setCharacterSize(fontSize);
    fpsText.setFillColor(sf::Color::Red);
    fpsText.setFont(guiFont);

    // Set up paused text.
    pausedText.setCharacterSize(fontSize + 10);
    pausedText.setFillColor(sf::Color::Blue);
    pausedText.setFont(guiFont);
    pausedText.setString("PAUSED!");

    // Set up health display text and bar.
    healthText.setCharacterSize(24);
    healthText.setFillColor(sf::Color::White);
    healthText.setFont(guiFont);
    healthText.setPosition(12.f, 12.f);
    healthText.setString("Health: 100/100");

    healthBarBg.setSize({ 200.f, 20.f });
    healthBarBg.setFillColor(sf::Color(50, 50, 50));
    healthBarBg.setOutlineColor(sf::Color::White);
    healthBarBg.setOutlineThickness(2.f);
    healthBarBg.setPosition(10.f, 40.f);

    healthBar.setSize({ 200.f, 20.f });
    healthBar.setFillColor(sf::Color::Green);
    healthBar.setPosition(10.f, 40.f);

    // Menu UI
    menuTitleText.setFont(guiFont);
    menuTitleText.setCharacterSize(80);
    menuTitleText.setFillColor(sf::Color::Red);
    menuTitleText.setString("Dwarf VS Mushrooms");

    menuPromptText.setFont(guiFont);
    menuPromptText.setCharacterSize(36);
    menuPromptText.setFillColor(sf::Color::Cyan);
    menuPromptText.setString("Press Enter to Start");
}

void Window::setup(const std::string& title, const sf::Vector2u& size)
{
    windowTitle = title;
    windowSize = size;
    create();
}

void Window::create()
{
    auto style = (isFullscreen ? sf::Style::Fullscreen : sf::Style::Default);

    if (isFullscreen) {
        auto dm = sf::VideoMode::getDesktopMode();
        window.create(dm, windowTitle, style);
    } else {
        window.create({ windowSize.x, windowSize.y, 32 }, windowTitle, style);
    }

    // Initialize main view with logical size (fallback to current window size)
    sf::Vector2u logical = logicalViewSize;
    if (logical.x == 0u || logical.y == 0u) {
        auto actual = window.getSize();
        logical = { actual.x, actual.y };
    }
    gameView = sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(logical.x), static_cast<float>(logical.y)));
    applyLetterboxToView(gameView, window.getSize().x, window.getSize().y);
    window.setView(gameView);

    // Place UI in logical coordinate space
    float Lx = static_cast<float>(logical.x);
    float Ly = static_cast<float>(logical.y);
    pausedText.setPosition(Lx * 0.5f - pausedText.getLocalBounds().width * 0.5f, 10.0f);

    // Center menu texts
    sf::FloatRect titleBounds = menuTitleText.getLocalBounds();
    menuTitleText.setPosition(Lx * 0.5f - titleBounds.width * 0.5f, Ly * 0.3f);
    sf::FloatRect promptBounds = menuPromptText.getLocalBounds();
    menuPromptText.setPosition(Lx * 0.5f - promptBounds.width * 0.5f, Ly * 0.5f);
}

void Window::destroy()
{
    window.close();
}

void Window::update()
{
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            isDone = true;
        else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F5)
            toggleFullscreen();
        else if (event.type == sf::Event::Resized)
            onResized(event.size.width, event.size.height);
    }
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
const sf::Vector2u& Window::getWindowSize() const { return windowSize; }
const sf::Font& Window::getGUIFont() const { return guiFont; }
sf::Text& Window::getFPSText() { return fpsText; }

void Window::drawGUI(const Game& game)
{
    // If in menu, draw menu and return
    if (game.isInMenu()) {
        window.draw(menuTitleText);
        window.draw(menuPromptText);
        return;
    }

    window.draw(fpsText);
    if (game.getPlayer()) {
        auto playerHealth = game.getPlayer()->getHealthComp()->getHealth();
        auto maxHealth = game.getPlayer()->getHealthComp()->getMaxHealth();
        std::ostringstream ss;
        ss << "Health: " << playerHealth << "/" << maxHealth;
        healthText.setString(ss.str());

        // Update health bar width and color
        float ratio = maxHealth > 0 ? static_cast<float>(playerHealth) / static_cast<float>(maxHealth) : 0.f;
        ratio = std::max(0.f, std::min(1.f, ratio));
        healthBar.setSize({ 200.f * ratio, 20.f });
        if (ratio > 0.6f) healthBar.setFillColor(sf::Color::Green);
        else if (ratio > 0.3f) healthBar.setFillColor(sf::Color::Yellow);
        else healthBar.setFillColor(sf::Color::Red);

        window.draw(healthText);
        window.draw(healthBarBg);
        window.draw(healthBar);

        // Draw 5 potion icons as segmented health display
        int segments = 5;
        int filled = static_cast<int>(std::ceil(ratio * segments));
        float iconScale = 0.6f;
        float iconSize = 50.f * iconScale; // source is 50x50 in project assets
        float startX = 10.f;
        float startY = 70.f;
        for (int i = 0; i < segments; ++i) {
            if (potionIconTexture.getSize().x > 0) {
                sf::Sprite spr;
                spr.setTexture(potionIconTexture);
                spr.setScale(iconScale, iconScale);
                spr.setPosition(startX + i * (iconSize + 8.f), startY);
                if (i < filled) {
                    spr.setColor(sf::Color(255, 255, 255, 255));
                } else {
                    spr.setColor(sf::Color(255, 255, 255, 80));
                }
                window.draw(spr);
            }
        }
    }
    if (game.isPaused()) {
        window.draw(pausedText);
    }
}

void Window::draw(sf::Drawable& drawable) {
    window.draw(drawable);
}


void Window::applyLetterbox(unsigned int windowWidth, unsigned int windowHeight)
{
    applyLetterboxToView(gameView, windowWidth, windowHeight);
}

void Window::onResized(unsigned int w, unsigned int h)
{
    windowSize = { w, h };
    applyLetterbox(w, h);
    window.setView(gameView);
}
