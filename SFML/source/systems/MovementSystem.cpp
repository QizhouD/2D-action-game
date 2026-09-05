#include "../../include/systems/Systems.h"
#include "../../include/components/VelocityComponent.h"
#include "../../include/components/PositionComponent.h"
#include "../../include/entities/Entity.h"
#include "../../include/core/Game.h"
#include "../../include/core/Board.h"
#include <stdexcept>

MovementSystem::MovementSystem() {
    componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::VELOCITY));
    componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::POSITION));
}

void MovementSystem::update(Game* game, Entity* entity, float elapsed) {
    auto velComp = std::dynamic_pointer_cast<VelocityComponent>(entity->getComponent(ComponentID::VELOCITY));
    auto posComp = std::dynamic_pointer_cast<PositionComponent>(entity->getComponent(ComponentID::POSITION));

    if (!velComp || !posComp) {
        throw std::runtime_error("MovementSystem: missing Velocity or Position component");
    }

    const sf::Vector2f v = velComp->getWorldVelocity();
    if (v.x == 0.f && v.y == 0.f) return;

    sf::Vector2f pos = posComp->getPosition();
    const Board* board = game->getBoard();

    if (!board || !entity->blocksOnWalls()) {
        posComp->setPosition(pos.x + v.x * elapsed, pos.y + v.y * elapsed);
        return;
    }

    bool blocked = false;

    // X axis
    if (v.x != 0.f) {
        const float nx = pos.x + v.x * elapsed;
        if (board->isBoxWalkable(entity->hitboxAt(nx, pos.y))) pos.x = nx;
        else blocked = true;
    }
    // Y axis
    if (v.y != 0.f) {
        const float ny = pos.y + v.y * elapsed;
        if (board->isBoxWalkable(entity->hitboxAt(pos.x, ny))) pos.y = ny;
        else blocked = true;
    }

    posComp->setPosition(pos);
    if (blocked) entity->onWallHit();
}
