#include "../../include/graphics/Hud.h"
#include "../../include/graphics/Window.h"
#include "../../include/core/Game.h"
#include "../../include/entities/Player.h"
#include <algorithm>
#include <cmath>
#include <sstream>

void Hud::init(const sf::Font& f, const std::string& woodIconFile)
{
    font = &f;
    if (woodIcon.loadFromFile(woodIconFile)) {
        woodSprite.setTexture(woodIcon);
        woodSprite.setScale(0.7f, 0.7f);
    }
}

void Hud::update(float elapsed)
{
    pulse += elapsed;
    if (damageFlash > 0.f) damageFlash -= elapsed;
    for (auto& t : toasts) t.remaining -= elapsed;
    while (!toasts.empty() && toasts.front().remaining <= 0.f) toasts.pop_front();
}

void Hud::pushToast(const std::string& text, float seconds)
{
    toasts.push_back({ text, seconds, seconds });
    if (toasts.size() > 4) toasts.pop_front();
}

void Hud::drawText(Window& window, const std::string& s, float x, float y, unsigned size,
                   sf::Color color, bool centered, bool shadow)
{
    if (!font) return;
    sf::Text text(s, *font, size);
    if (centered) {
        const auto b = text.getLocalBounds();
        text.setOrigin(b.left + b.width * 0.5f, b.top);
    }
    if (shadow) {
        text.setFillColor(sf::Color(0, 0, 0, 180));
        text.setPosition(x + 3.f, y + 3.f);
        window.draw(text);
    }
    text.setFillColor(color);
    text.setPosition(x, y);
    window.draw(text);
}

void Hud::drawDim(Window& window, const sf::Vector2f& size, sf::Uint8 alpha)
{
    sf::RectangleShape dim(size);
    dim.setFillColor(sf::Color(0, 0, 0, alpha));
    window.draw(dim);
}

void Hud::drawToasts(Window& window, const sf::Vector2f& logical)
{
    float y = logical.y * 0.16f;
    for (const auto& t : toasts) {
        const float fade = std::min(1.f, t.remaining / 0.5f);
        const sf::Uint8 a = static_cast<sf::Uint8>(255 * fade);
        sf::Text text(t.text, *font, subtitleSize);
        const auto b = text.getLocalBounds();
        sf::RectangleShape bg({ b.width + 60.f, b.height + 40.f });
        bg.setOrigin(bg.getSize().x * 0.5f, 0.f);
        bg.setPosition(logical.x * 0.5f, y);
        bg.setFillColor(sf::Color(20, 20, 30, static_cast<sf::Uint8>(190 * fade)));
        bg.setOutlineThickness(3.f);
        bg.setOutlineColor(sf::Color(255, 210, 60, a));
        window.draw(bg);
        drawText(window, t.text, logical.x * 0.5f, y + 8.f, subtitleSize, sf::Color(255, 235, 150, a), true, false);
        y += bg.getSize().y + 14.f;
    }
}

void Hud::drawGameplay(Window& window, const Game& game)
{
    if (!font) return;
    const sf::Vector2f logical(static_cast<float>(window.getLogicalSize().x),
                               static_cast<float>(window.getLogicalSize().y));
    auto player = game.getPlayer();

    // --- Health bar --------------------------------------------------------
    if (player) {
        const int hp = player->getHealthComp()->getHealth();
        const int maxHp = player->getHealthComp()->getMaxHealth();
        const float ratio = maxHp > 0 ? static_cast<float>(hp) / maxHp : 0.f;
        const sf::Vector2f barPos(20.f, 20.f), barSize(320.f, 30.f);

        sf::RectangleShape bg(barSize);
        bg.setPosition(barPos);
        bg.setFillColor(sf::Color(30, 30, 30, 220));
        bg.setOutlineThickness(3.f);
        bg.setOutlineColor(sf::Color(230, 230, 230));
        window.draw(bg);

        sf::RectangleShape fill({ barSize.x * ratio, barSize.y });
        fill.setPosition(barPos);
        const sf::Uint8 r = static_cast<sf::Uint8>(255 * (1.f - ratio));
        const sf::Uint8 g = static_cast<sf::Uint8>(200 * ratio + 30);
        fill.setFillColor(sf::Color(r, g, 40));
        window.draw(fill);

        std::ostringstream hpText;
        hpText << "HP " << hp << " / " << maxHp;
        drawText(window, hpText.str(), barPos.x + 10.f, barPos.y - 10.f, hudSize, sf::Color::White);

        // --- Wood ----------------------------------------------------------
        woodSprite.setPosition(20.f, 62.f);
        window.draw(woodSprite);
        drawText(window, "x " + std::to_string(player->getWood()), 66.f, 56.f, hudSize, sf::Color(255, 220, 160));
    }

    // --- Level / enemies ---------------------------------------------------
    {
        std::ostringstream lvl;
        lvl << "Level " << (game.getLevelIndex() + 1) << " / " << game.getLevelCount();
        drawText(window, lvl.str(), logical.x * 0.5f, 14.f, hudSize, sf::Color(220, 220, 255), true);

        const int enemies = game.getEnemiesAlive();
        std::string enemyText = enemies > 0 ? "Enemies: " + std::to_string(enemies)
                                            : (game.getBoard() && game.getBoard()->hasExit() ? "Exit open!" : "Clear!");
        sf::Color c = enemies > 0 ? sf::Color(255, 140, 140) : sf::Color(255, 230, 120);
        sf::Text probe(enemyText, *font, hudSize);
        drawText(window, enemyText, logical.x - probe.getLocalBounds().width - 30.f, 14.f, hudSize, c);

        drawText(window, "Score " + std::to_string(game.getScore()),
                 logical.x - 30.f - sf::Text("Score " + std::to_string(game.getScore()), *font, hudSize).getLocalBounds().width,
                 56.f, hudSize, sf::Color(200, 255, 200));
    }

    if (window.isDebugDraw()) {
        drawText(window, "FPS " + std::to_string(fps_), 20.f, logical.y - 60.f, hudSize, sf::Color::Red);
    }

    // --- Damage flash --------------------------------------------------------
    if (damageFlash > 0.f) {
        sf::RectangleShape flash(logical);
        flash.setFillColor(sf::Color(200, 0, 0, static_cast<sf::Uint8>(130 * damageFlash / damageFlashDuration)));
        window.draw(flash);
    }

    drawToasts(window, logical);
}

void Hud::drawOverlay(Window& window, const Game& game)
{
    if (!font) return;
    const sf::Vector2f logical(static_cast<float>(window.getLogicalSize().x),
                               static_cast<float>(window.getLogicalSize().y));
    const float cx = logical.x * 0.5f;
    const bool blink = std::fmod(pulse, 1.0f) < 0.6f;
    const sf::Color prompt = blink ? sf::Color(255, 240, 180) : sf::Color(255, 240, 180, 120);

    switch (game.getState()) {
    case GameState::MainMenu:
        drawDim(window, logical, 170);
        drawText(window, "DWARF & FIRE", cx, logical.y * 0.16f, titleSize, sf::Color(255, 200, 80), true);
        drawText(window, "A tiny action game", cx, logical.y * 0.16f + 140.f, subtitleSize, sf::Color(220, 220, 220), true);
        drawText(window, "WASD / Arrows  -  move    (Tab toggles scheme)", cx, logical.y * 0.42f, hudSize, sf::Color::White, true);
        drawText(window, "Space  -  swing axe: chop logs, hit enemies", cx, logical.y * 0.42f + 50.f, hudSize, sf::Color::White, true);
        drawText(window, "Left Shift  -  spend 1 wood to throw a fireball", cx, logical.y * 0.42f + 100.f, hudSize, sf::Color::White, true);
        drawText(window, "Kill every mushroom, then step on the golden exit", cx, logical.y * 0.42f + 150.f, hudSize, sf::Color(255, 220, 160), true);
        drawText(window, "Esc pause   F1 debug   F5 fullscreen", cx, logical.y * 0.42f + 200.f, hudSize, sf::Color(170, 170, 170), true);
        drawText(window, "Press ENTER to start", cx, logical.y * 0.78f, subtitleSize, prompt, true);
        break;

    case GameState::Paused:
        drawDim(window, logical, 140);
        drawText(window, "PAUSED", cx, logical.y * 0.3f, titleSize, sf::Color(120, 170, 255), true);
        drawText(window, "Esc  resume", cx, logical.y * 0.3f + 170.f, subtitleSize, sf::Color::White, true);
        drawText(window, "R  restart level", cx, logical.y * 0.3f + 230.f, subtitleSize, sf::Color::White, true);
        drawText(window, "Q  quit to menu", cx, logical.y * 0.3f + 290.f, subtitleSize, sf::Color::White, true);
        break;

    case GameState::LevelClear:
        drawDim(window, logical, 150);
        drawText(window, "LEVEL CLEAR!", cx, logical.y * 0.28f, titleSize, sf::Color(255, 220, 80), true);
        drawText(window, "Score " + std::to_string(game.getScore()) + "    Kills " + std::to_string(game.getKills()),
                 cx, logical.y * 0.28f + 170.f, subtitleSize, sf::Color::White, true);
        drawText(window, "Press ENTER for the next level", cx, logical.y * 0.7f, subtitleSize, prompt, true);
        break;

    case GameState::GameOver:
        drawDim(window, logical, 180);
        drawText(window, "YOU DIED", cx, logical.y * 0.28f, titleSize, sf::Color(255, 70, 70), true);
        drawText(window, "Score " + std::to_string(game.getScore()) + "    Kills " + std::to_string(game.getKills()),
                 cx, logical.y * 0.28f + 170.f, subtitleSize, sf::Color::White, true);
        drawText(window, "R  retry level", cx, logical.y * 0.66f, subtitleSize, prompt, true);
        drawText(window, "ENTER  back to menu", cx, logical.y * 0.66f + 60.f, subtitleSize, sf::Color(200, 200, 200), true);
        break;

    case GameState::Victory:
        drawDim(window, logical, 170);
        drawText(window, "VICTORY!", cx, logical.y * 0.26f, titleSize, sf::Color(255, 230, 90), true);
        drawText(window, "The Mushroom King is no more.", cx, logical.y * 0.26f + 160.f, subtitleSize, sf::Color::White, true);
        drawText(window, "Final score " + std::to_string(game.getScore()) + "    Kills " + std::to_string(game.getKills()),
                 cx, logical.y * 0.26f + 230.f, subtitleSize, sf::Color(200, 255, 200), true);
        drawText(window, "Press ENTER to return to the menu", cx, logical.y * 0.74f, subtitleSize, prompt, true);
        break;

    case GameState::Playing:
    default:
        break;
    }
}
