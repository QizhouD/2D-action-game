#include "../../include/entities/Mushroom.h"
#include "../../include/core/Game.h"
#include <cmath>

Mushroom::Mushroom() : Entity(EntityType::MUSHROOM) {
    health = std::make_shared<HealthComponent>(defaultHealth, defaultHealth);
    velocity = std::make_shared<VelocityComponent>(moveSpeed);
    // Start with an empty collider; it will be updated by ColliderSystem using the entity's size
    Rectangle initRect; // default 0-sized; ColliderSystem updates it based on sprite size
    collider = std::make_shared<ColliderComponent>(initRect);

    addComponent(health);
    addComponent(velocity);
    addComponent(collider);
}

void Mushroom::init(const std::string& textureFile, float scale) {
    Entity::init(textureFile, scale);
}

void Mushroom::update(Game* game, float elapsed) {
    // Simple chase AI: move toward player position
    auto player = game->getPlayer();
    if (player) {
        auto myPos = getPosition();
        auto pPos = player->getPosition();
        sf::Vector2f dir{ pPos.x - myPos.x, pPos.y - myPos.y };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.001f) {
            dir.x /= len; dir.y /= len;
            velocity->setVelocity(dir.x, dir.y);
        } else {
            velocity->setVelocity(0, 0);
        }
    }

    // Call base to sync sprite/bbox
    Entity::update(game, elapsed);
}
