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

    // Settlement/Result overlay has priority
    if (game.isInResult()) {
        // Softer background dim
        sf::RectangleShape overlay;
        overlay.setSize(sf::Vector2f(static_cast<float>(logicalViewSize.x), static_cast<float>(logicalViewSize.y)));
        overlay.setFillColor(sf::Color(0, 0, 0, 140));
        window.draw(overlay);

        // Themed panel with subtle shadow (forest/dwarf vibes)
        float pw = std::min(720.f, static_cast<float>(logicalViewSize.x) * 0.78f);
        float ph = 360.f;
        float px = (logicalViewSize.x - pw) * 0.5f;
        float py = (logicalViewSize.y - ph) * 0.5f;

        // Shadow
        sf::RectangleShape shadow;
        shadow.setSize({ pw, ph });
        shadow.setFillColor(sf::Color(0, 0, 0, 100));
        shadow.setPosition(px + 6.f, py + 6.f);
        window.draw(shadow);

        // Panel
        sf::RectangleShape panel;
        panel.setSize({ pw, ph });
        panel.setFillColor(sf::Color(22, 24, 28, 230));
        panel.setOutlineThickness(0.f);
        panel.setPosition(px, py);
        window.draw(panel);

        // Load small icons once (cache)
        static sf::Texture trophyTex; static bool trophyLoaded = false;
        static sf::Texture mushTex; static bool mushLoaded = false;
        static sf::Texture fireTex; static bool fireLoaded = false;
        if (!trophyLoaded) { trophyLoaded = trophyTex.loadFromFile("img/checkpoint.png"); }
        if (!mushLoaded)   { mushLoaded   = mushTex.loadFromFile("img/mushroom50-50.png"); }
        if (!fireLoaded)   { fireLoaded   = fireTex.loadFromFile("img/fire.png"); }

        // Title with soft shadow
        sf::Text title("Victory!", guiFont, 64);
        title.setFillColor(sf::Color(245, 245, 245));
        sf::Text titleShadow = title; titleShadow.setFillColor(sf::Color(0, 0, 0, 120));
        // center
        float titleX = px + (pw - title.getLocalBounds().width) * 0.5f;
        float titleY = py + 22.f;
        titleShadow.setPosition(titleX + 2.f, titleY + 2.f);
        title.setPosition(titleX, titleY);

        // Optional emblem left of title
        if (trophyLoaded) {
            sf::Sprite emblem(trophyTex);
            float eScale = 0.9f;
            emblem.setScale(eScale, eScale);
            float eY = titleY - 8.f;
            float eX = titleX - 56.f; // slightly left to title
            emblem.setPosition(eX, eY);
            window.draw(emblem);
        }
        window.draw(titleShadow);
        window.draw(title);

        // Subtitle
        sf::Text subtitle("Level complete", guiFont, 28);
        subtitle.setFillColor(sf::Color(200, 210, 210));
        subtitle.setPosition(px + (pw - subtitle.getLocalBounds().width) * 0.5f, titleY + 56.f);
        window.draw(subtitle);

        // Stats with icons
        float left = px + 36.f;
        float rowY = py + 120.f;
        float lineH = 46.f;
        auto drawStat = [&](const std::string& label, const std::string& value, const sf::Texture* icon){
            // icon
            float iconW = 36.f, iconH = 36.f;
            float textX = left + (icon ? (iconW + 14.f) : 0.f);
            if (icon && icon->getSize().x > 0) {
                sf::Sprite s(*icon);
                float scaleX = iconW / static_cast<float>(icon->getSize().x);
                float scaleY = iconH / static_cast<float>(icon->getSize().y);
                s.setScale(scaleX, scaleY);
                s.setPosition(left, rowY - 4.f);
                window.draw(s);
            }
            // label/value
            sf::Text line(label + ": " + value, guiFont, 34);
            line.setFillColor(sf::Color(230, 230, 230));
            sf::Text lineShadow = line; lineShadow.setFillColor(sf::Color(0, 0, 0, 120));
            lineShadow.setPosition(textX + 2.f, rowY + 2.f);
            line.setPosition(textX, rowY);
            window.draw(lineShadow);
            window.draw(line);
            rowY += lineH;
        };

        // Format time to 2 decimals
        std::ostringstream tss; tss.setf(std::ios::fixed); tss.precision(2);
        tss << game.getLevelFinishSeconds() << " s";
        drawStat("Time", tss.str(), trophyLoaded ? &trophyTex : nullptr);

        std::ostringstream kss; kss << game.getKillCount();
        drawStat("Kills", kss.str(), mushLoaded ? &mushTex : nullptr);

        std::ostringstream fss; fss << game.getFireShotCount();
        drawStat("Fire shots", fss.str(), fireLoaded ? &fireTex : nullptr);

        // Hint line for navigation
        sf::Text hint("Press Enter to return to Menu  |  Press R to Restart", guiFont, 24);
        hint.setFillColor(sf::Color(190, 190, 190));
        hint.setPosition(px + (pw - hint.getLocalBounds().width) * 0.5f, py + ph - 40.f);
        window.draw(hint);

        return; // don't draw regular HUD
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
