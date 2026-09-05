#include "../../include/components/InputComponent.h"
#include "../../include/core/Game.h"
#include "../../include/core/InputHandler.h"
#include "../../include/entities/Player.h"
#include <cmath>
#include <iostream>

PlayerInputComponent::PlayerInputComponent()
{
    inputHandler = std::make_unique<PlayerInputHandler>();
}

PlayerInputComponent::~PlayerInputComponent() = default;

void PlayerInputComponent::update(Game& game)
{
    auto player = game.getPlayer();
    if (!player) { return; }

    auto velComp = player->getVelocityComp();
    if (!velComp) { return; }

    // Movement is rebuilt from scratch every tick.
    velComp->setVelocity(0.f, 0.f);

    // Ignore the keyboard while the window is in the background.
    if (!game.getWindow()->hasFocus()) { return; }

    auto& commands = inputHandler->handleInput();
    for (auto& cmd : commands) {
        cmd->execute(game);
    }

    // Normalise so diagonal movement is not sqrt(2) times faster.
    sf::Vector2f dir = velComp->getVelocity();
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 1.f) {
        velComp->setVelocity(dir.x / len, dir.y / len);
    }
}
