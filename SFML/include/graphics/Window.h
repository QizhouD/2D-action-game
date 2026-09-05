#pragma once
#include <SFML/Graphics.hpp>
#include <set>
#include <string>
#include <vector>

// Thin wrapper around sf::RenderWindow.
//
// Coordinates:
//   * world size  - the level in pixels (tiles * tile size)
//   * view size   - how much of the world is visible at once (<= maxView);
//                   small levels are shown whole, big ones scroll with the camera
//   * pixel size  - the OS window client area; a letter-boxed sf::View maps
//                   the view onto it, so any window size/aspect works
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
    const sf::Font& getGUIFont() const { return guiFont; }

    // --- World / camera ---------------------------------------------------------
    // Sets the level size. The visible area becomes min(world, maxView).
    void setWorld(const sf::Vector2u& worldSize);
    const sf::Vector2u& getWorldSize() const { return worldSize; }
    // Size of the visible area in world units (also the UI coordinate space).
    const sf::Vector2u& getLogicalSize() const { return viewSize; }
    // Smoothly moves the camera towards `target` (clamped to the world).
    void followCamera(const sf::Vector2f& target, float elapsed);
    void snapCamera(const sf::Vector2f& target);
    // Switch between world-space (scrolling) and UI-space (fixed) drawing.
    void beginWorld();
    void beginUI();
    sf::FloatRect getVisibleWorldRect() const;

    // --- Input ----------------------------------------------------------------
    // All keyboard/gamepad reads go through here so they can be replaced by a script.
    bool wasKeyPressed(sf::Keyboard::Key key) const;   // edge
    bool isKeyDown(sf::Keyboard::Key key) const;       // level
    bool wasJoyButtonPressed(unsigned button) const;   // edge, joystick 0
    bool isJoyButtonDown(unsigned button) const;       // level, joystick 0
    // Left stick + d-pad, each axis in [-1, 1] with a dead zone applied.
    sf::Vector2f getJoyDirection() const;
    bool isJoystickConnected() const;

    // --- Automation -----------------------------------------------------------
    // Drives the game from a text script instead of the real keyboard:
    //   <seconds> down <Key>      hold a key from this time on
    //   <seconds> up <Key>        release it
    //   <seconds> press <Key>     down+up within one frame
    //   <seconds> shot <file.png> save the next rendered frame
    //   <seconds> quit
    // Times are relative to when the script was loaded. Lines starting with #
    // are ignored. Key names follow sf::Keyboard (A..Z, Num0..9, Enter, Escape,
    // Space, LShift, Up, Down, Left, Right, F1, F5, Tab, M).
    bool loadScript(const std::string& file);
    bool isScripted() const { return scripted; }
    // Save the next frame (taken in endDraw, before display) to `file`.
    void requestScreenshot(const std::string& file) { pendingShot = file; }

    void toggleFullscreen();
    void draw(const sf::Drawable& drawable);
    void redraw();
    // Request shutdown; the main loop exits on the next iteration.
    void close() { isDone = true; }

    // Creates the OS window sized for the given view.
    void setup(const std::string& title, const sf::Vector2u& viewSize);
    inline void setTitle(const std::string& t) { windowTitle = t; }

    void setDebugDraw(bool on) { debugDraw = on; }
    bool isDebugDraw() const { return debugDraw; }

private:
    struct ScriptEvent { float time; std::string action; std::string arg; };

    void destroy();
    void create();
    void applyViewport(sf::View& view) const;
    void clampCamera();
    void advanceScript();
    void takeScreenshot(const std::string& file);

    sf::RenderWindow window;
    sf::Vector2u worldSize;     // level size in world units
    sf::Vector2u viewSize;      // visible area in world units
    sf::Vector2u maxView;       // upper bound for viewSize
    sf::Vector2u pixelSize;     // actual client area
    sf::Vector2f camera;        // centre of the visible area, world units
    std::string windowTitle;
    sf::Font guiFont;

    std::vector<sf::Keyboard::Key> keysPressed;
    std::vector<unsigned> joyPressed;

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
