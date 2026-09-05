#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
class Game;

// Thin wrapper around sf::RenderWindow.
//
// The game works in a fixed "logical" resolution (the tile map size in pixels).
// The actual OS window may be smaller (screen too small) or larger (fullscreen);
// a letter-boxed sf::View maps the logical space onto whatever pixel size we got.
class Window {
public:
    Window();
    ~Window();

    void loadFont(const std::string& fontFile);
    void beginDraw();
    void endDraw();

    // Pump the OS event queue. Call once per frame, before reading input.
    void pollEvents();

    bool isWindowDone() const;
    bool isWindowFullscreen() const;
    bool hasFocus() const { return focused; }
    const sf::Vector2u& getLogicalSize() const { return logicalSize; }
    const sf::Font& getGUIFont() const { return guiFont; }

    // True if the key went down during the last pollEvents() (edge, not level).
    bool wasKeyPressed(sf::Keyboard::Key key) const;

    void toggleFullscreen();
    void draw(const sf::Drawable& drawable);
    void redraw();
    void drawGUI(const Game& game);

    // Creates the OS window for the given logical size.
    void setup(const std::string& title, const sf::Vector2u& logicalSize);
    inline void setTitle(const std::string& t) { windowTitle = t; }

    void setFPS(int fps);
    void setDebugDraw(bool on) { debugDraw = on; }
    bool isDebugDraw() const { return debugDraw; }

private:
    const int fontSize = 50;

    void destroy();
    void create();
    void applyView();

    sf::RenderWindow window;
    sf::Vector2u logicalSize;   // game units
    sf::Vector2u pixelSize;     // actual client area
    std::string windowTitle;
    sf::Font guiFont;
    sf::Text fpsText;
    sf::Text pausedText;
    sf::Text healthText;

    std::vector<sf::Keyboard::Key> keysPressed;

    bool isDone;
    bool isFullscreen;
    bool focused;
    bool debugDraw;
};
