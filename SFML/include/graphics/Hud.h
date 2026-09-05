#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>

class Game;
class Window;

// All on-screen UI: in-game bars/counters, achievement toasts, the damage
// flash, and the full-screen overlays for menu / pause / game over / victory.
class Hud {
public:
    void init(const sf::Font& font, const std::string& woodIconFile);
    void update(float elapsed);

    void pushToast(const std::string& text, float seconds = 2.5f);
    void flashDamage() { damageFlash = damageFlashDuration; }
    void clearToasts() { toasts.clear(); }

    // In-game layer (HP, wood, enemies, level, FPS when debugging).
    void drawGameplay(Window& window, const Game& game);
    // State-dependent overlay (menu, paused, level clear, game over, victory).
    void drawOverlay(Window& window, const Game& game);

    void setFPS(int fps) { fps_ = fps; }

private:
    struct Toast { std::string text; float remaining; float total; };

    void drawText(Window& window, const std::string& s, float x, float y, unsigned size,
                  sf::Color color, bool centered = false, bool shadow = true);
    void drawDim(Window& window, const sf::Vector2f& size, sf::Uint8 alpha);
    void drawToasts(Window& window, const sf::Vector2f& logical);

    const sf::Font* font = nullptr;
    sf::Texture woodIcon;
    sf::Sprite woodSprite;
    std::deque<Toast> toasts;
    float damageFlash = 0.f;
    float pulse = 0.f;      // for blinking prompts
    int fps_ = 0;

    static constexpr float damageFlashDuration = 0.25f;
    static constexpr unsigned hudSize = 40;
    static constexpr unsigned titleSize = 130;
    static constexpr unsigned subtitleSize = 52;
};
