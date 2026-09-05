#include "../../include/graphics/Window.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {
    sf::Keyboard::Key keyFromName(const std::string& name)
    {
        static const std::unordered_map<std::string, sf::Keyboard::Key> named = {
            { "Enter", sf::Keyboard::Enter }, { "Escape", sf::Keyboard::Escape }, { "Esc", sf::Keyboard::Escape },
            { "Space", sf::Keyboard::Space }, { "LShift", sf::Keyboard::LShift }, { "RShift", sf::Keyboard::RShift },
            { "Up", sf::Keyboard::Up }, { "Down", sf::Keyboard::Down }, { "Left", sf::Keyboard::Left }, { "Right", sf::Keyboard::Right },
            { "F1", sf::Keyboard::F1 }, { "F5", sf::Keyboard::F5 }, { "Tab", sf::Keyboard::Tab },
        };
        auto it = named.find(name);
        if (it != named.end()) return it->second;
        if (name.size() == 1 && name[0] >= 'A' && name[0] <= 'Z')
            return static_cast<sf::Keyboard::Key>(sf::Keyboard::A + (name[0] - 'A'));
        if (name.size() == 1 && name[0] >= 'a' && name[0] <= 'z')
            return static_cast<sf::Keyboard::Key>(sf::Keyboard::A + (name[0] - 'a'));
        if (name.size() == 4 && name.rfind("Num", 0) == 0 && name[3] >= '0' && name[3] <= '9')
            return static_cast<sf::Keyboard::Key>(sf::Keyboard::Num0 + (name[3] - '0'));
        return sf::Keyboard::Unknown;
    }
}

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
}

void Window::setup(const std::string& title, const sf::Vector2u& size)
{
    windowTitle = title;
    logicalSize = size;
    create();
}

void Window::setLogicalSize(const sf::Vector2u& size)
{
    logicalSize = size;
    applyView();
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
}

void Window::applyView()
{
    if (logicalSize.x == 0 || logicalSize.y == 0 || pixelSize.x == 0 || pixelSize.y == 0) return;

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
    scriptTapped.clear();

    sf::Event event;
    while (window.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::Closed:
            isDone = true;
            break;
        case sf::Event::KeyPressed:
            if (scripted) break;               // real keyboard is ignored while scripted
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

    if (scripted) advanceScript();
}

bool Window::wasKeyPressed(sf::Keyboard::Key key) const
{
    return std::find(keysPressed.begin(), keysPressed.end(), key) != keysPressed.end();
}

bool Window::isKeyDown(sf::Keyboard::Key key) const
{
    if (scripted) return scriptHeld.count(key) > 0 || scriptTapped.count(key) > 0;
    return sf::Keyboard::isKeyPressed(key);
}

// ---------------------------------------------------------------------------
// Scripted input
// ---------------------------------------------------------------------------

bool Window::loadScript(const std::string& file)
{
    std::ifstream in(file);
    if (!in) {
        std::cerr << "[Window] script not found: " << file << "\n";
        return false;
    }
    script.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        ScriptEvent ev;
        if (!(ss >> ev.time >> ev.action)) continue;
        ss >> ev.arg;
        script.push_back(ev);
    }
    std::stable_sort(script.begin(), script.end(),
        [](const ScriptEvent& a, const ScriptEvent& b) { return a.time < b.time; });
    scriptPos = 0;
    scripted = true;
    scriptClock.restart();
    std::cout << "[Window] running script " << file << " (" << script.size() << " events)\n";
    return true;
}

void Window::advanceScript()
{
    const float now = scriptClock.getElapsedTime().asSeconds();
    while (scriptPos < script.size() && script[scriptPos].time <= now) {
        const ScriptEvent& ev = script[scriptPos++];
        const sf::Keyboard::Key key = keyFromName(ev.arg);

        if (ev.action == "down")       { scriptHeld.insert(key); keysPressed.push_back(key); }
        else if (ev.action == "up")    { scriptHeld.erase(key); }
        else if (ev.action == "press") { scriptTapped.insert(key); keysPressed.push_back(key); }
        else if (ev.action == "shot")  { requestScreenshot(ev.arg); }
        else if (ev.action == "quit")  { isDone = true; }
        else std::cerr << "[Window] unknown script action: " << ev.action << "\n";
    }
}

void Window::takeScreenshot(const std::string& file)
{
    sf::Texture tex;
    if (!tex.create(window.getSize().x, window.getSize().y)) return;
    tex.update(window);
    if (tex.copyToImage().saveToFile(file))
        std::cout << "[Window] screenshot saved: " << file << "\n";
    else
        std::cerr << "[Window] failed to save screenshot: " << file << "\n";
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

void Window::endDraw()
{
    if (!pendingShot.empty()) {
        // Capture the back buffer while it still holds the finished frame.
        takeScreenshot(pendingShot);
        pendingShot.clear();
    }
    window.display();
}

bool Window::isWindowDone() const { return isDone; }
bool Window::isWindowFullscreen() const { return isFullscreen; }

void Window::draw(const sf::Drawable& drawable) {
    window.draw(drawable);
}
