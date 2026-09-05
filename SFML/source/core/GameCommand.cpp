#include "../../include/core/Command.h"
#include "../../include/core/Game.h"
#include "../../include/entities/Player.h"
#include <SFML/Window/Keyboard.hpp>

void PauseCommand::execute(Game& game)
{
    game.togglePause();
}

// Movement commands write a unit direction into the player's velocity component.
// PlayerInputComponent normalises the combined direction and the component's
// speed factor turns it into pixels per second, so diagonals are not faster.
namespace {
    void addDirection(Game& game, float dx, float dy)
    {
        auto player = game.getPlayer();
        if (!player) return;
        auto vcomp = player->getVelocityComp();
        if (!vcomp) return;
        sf::Vector2f v = vcomp->getVelocity();
        vcomp->setVelocity(v.x + dx, v.y + dy);
    }
}

void MoveRightCommand::execute(Game& game) { addDirection(game,  1.f,  0.f); }
void MoveLeftCommand::execute(Game& game)  { addDirection(game, -1.f,  0.f); }
void MoveUpCommand::execute(Game& game)    { addDirection(game,  0.f, -1.f); }
void MoveDownCommand::execute(Game& game)  { addDirection(game,  0.f,  1.f); }

void AttackCommand::execute(Game& game)
{
    if (auto player = game.getPlayer()) player->startAttack();
}

void ShoutCommand::execute(Game& game)
{
    if (auto player = game.getPlayer()) player->startShout();
}
