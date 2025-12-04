#pragma once
#include <SFML/Graphics.hpp>
#include <string>
class Game;

class Window {
public:
    Window();
    ~Window();

    void loadFont(const std::string& fontFile);
    void beginDraw();
    void endDraw();

    void update();

    bool isWindowDone() const;
    bool isWindowFullscreen() const;
    const sf::Vector2u& getWindowSize() const;
    const sf::Font& getGUIFont() const;
    sf::Text& getFPSText();

    void toggleFullscreen();
    void draw(sf::Drawable& drawable);
    void redraw();
    void drawGUI(const Game& game);

    void setup(const std::string& title, const sf::Vector2u& size);
    inline void setTitle(const std::string& t) { windowTitle = t; }
    inline void setSize(const sf::Vector2u& size) { windowSize = size; }
    inline void setLogicalSize(const sf::Vector2u& size) { logicalViewSize = size; }

    // Handle resize to keep aspect ratio (letterbox)
    void onResized(unsigned int w, unsigned int h);

    // Debug draw toggle for green bounding boxes
    inline bool isDebugBoundsVisible() const { return showDebugBounds; }
    inline void toggleDebugBounds() { showDebugBounds = !showDebugBounds; }

private:
    const int fontSize = 50;

    void destroy();
    void create();

    // Applies letterbox to keep view's aspect ratio inside the window
    void applyLetterbox(unsigned int windowWidth, unsigned int windowHeight);

    sf::RenderWindow window;
    sf::Vector2u windowSize;
    std::string windowTitle;
    sf::Font guiFont;
    sf::Text fpsText;
    sf::Text pausedText;
    sf::Text healthText;
    sf::RectangleShape healthBarBg;
    sf::RectangleShape healthBar;

    // Menu UI
    sf::Text menuTitleText;
    sf::Text menuPromptText;

    // Potion icons for health segments UI
    sf::Texture potionIconTexture;

    // Main render view with a fixed logical size equal to windowSize (set by Game)
    sf::View gameView;
    sf::Vector2u logicalViewSize{0u, 0u};

    bool isDone;
    bool isFullscreen;

    // Controls whether green debug rectangles are drawn
    bool showDebugBounds = true;
};
