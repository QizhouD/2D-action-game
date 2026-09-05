#pragma once
#include <SFML/Graphics.hpp>
#include <set>
#include <string>
#include <vector>

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

    bool isCreated() const { return window.isOpen(); }
    bool isWindowDone() const;
    bool isWindowFullscreen() const;
    bool hasFocus() const { return scripted || focused; }
    const sf::Vector2u& getLogicalSize() const { return logicalSize; }
    const sf::Font& getGUIFont() const { return guiFont; }

    // --- Input ----------------------------------------------------------------
    // All keyboard reads go through here so they can be replaced by a script.
    // True if the key went down during the last pollEvents() (edge, not level).
    bool wasKeyPressed(sf::Keyboard::Key key) const;
    // True while the key is held (level).
    bool isKeyDown(sf::Keyboard::Key key) const;

    // --- Automation -----------------------------------------------------------
    // Drives the game from a text script instead of the real keyboard:
    //   <seconds> down <Key>      hold a key from this time on
    //   <seconds> up <Key>        release it
    //   <seconds> press <Key>     down+up within one frame
    //   <seconds> shot <file.png> save the next rendered frame
    //   <seconds> quit
    // Times are relative to when the script was loaded. Lines starting with #
    // are ignored. Key names follow sf::Keyboard (A..Z, Num0..9, Enter, Escape,
    // Space, LShift, Up, Down, Left, Right, F1, F5).
    bool loadScript(const std::string& file);
    bool isScripted() const { return scripted; }
    // Save the next frame (taken in endDraw, before display) to `file`.
    void requestScreenshot(const std::string& file) { pendingShot = file; }

    void toggleFullscreen();
    void draw(const sf::Drawable& drawable);
    void redraw();
    // Request shutdown; the main loop exits on the next iteration.
    void close() { isDone = true; }

    // Creates the OS window for the given logical size.
    void setup(const std::string& title, const sf::Vector2u& logicalSize);
    // Changes the logical size (e.g. a level with different dimensions) and
    // refits the view; the OS window keeps its pixel size.
    void setLogicalSize(const sf::Vector2u& size);
    inline void setTitle(const std::string& t) { windowTitle = t; }

    void setDebugDraw(bool on) { debugDraw = on; }
    bool isDebugDraw() const { return debugDraw; }

private:
    struct ScriptEvent { float time; std::string action; std::string arg; };

    void destroy();
    void create();
    void applyView();
    void advanceScript();
    void takeScreenshot(const std::string& file);

    sf::RenderWindow window;
    sf::Vector2u logicalSize;   // game units
    sf::Vector2u pixelSize;     // actual client area
    std::string windowTitle;
    sf::Font guiFont;

    std::vector<sf::Keyboard::Key> keysPressed;

    // scripted input
    bool scripted = false;
    std::vector<ScriptEvent> script;
    size_t scriptPos = 0;
    sf::Clock scriptClock;
    std::set<sf::Keyboard::Key> scriptHeld;
    std::set<sf::Keyboard::Key> scriptTapped;   // "press": down for exactly one frame
    std::string pendingShot;

    bool isDone;
    bool isFullscreen;
    bool focused;
    bool debugDraw;
};
