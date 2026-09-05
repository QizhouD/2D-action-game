#include "../../include/entities/Fire.h"
#include "../../include/components/TTLComponent.h"
#include "../../include/components/VelocityComponent.h"
#include "../../include/core/Game.h"
#include "../../include/graphics/ParticleSystem.h"
#include "../../include/core/Balance.h"
#include <cmath>

Fire::Fire() : Entity(EntityType::FIRE), damage(Balance::get().fire.damage) {
    const auto& cfg = Balance::get().fire;
    ttl = std::make_shared<TTLComponent>(cfg.ttlTicks);
    addComponent(ttl);

    // Direction is set by the spawner; the speed factor turns it into px/s.
    velocity = std::make_shared<VelocityComponent>(cfg.speed);
    addComponent(velocity);
}

Fire::~Fire() {}

void Fire::init(const std::string& textureFile, float scale) {
    Entity::init(textureFile, scale);
    // Rotate around the centre so the sprite can point in any direction while
    // the hitbox/position keep using the top-left corner.
    sprite.setOrigin(texture->getSize().x * 0.5f, texture->getSize().y * 0.5f);
    // Slightly smaller hitbox than the sprite: the flame art has soft edges.
    const sf::Vector2f size = getSpriteSize();
    setHitbox(size.x * 0.15f, size.y * 0.15f, size.x * 0.7f, size.y * 0.7f);
}

void Fire::setDirection(const sf::Vector2f& dir) {
    direction = dir;
    velocity->setVelocity(dir.x, dir.y);
    sprite.setRotation(std::atan2(dir.y, dir.x) * 180.f / 3.14159265f);
}

std::shared_ptr<TTLComponent> Fire::getTTLComponent() const {
    return ttl;
}

void Fire::onWallHit(Game* game) {
    if (isDeleted()) return;
    deleteEntity();
    if (game) {
        const sf::Vector2f back = -direction;
        game->getParticles().burst(getCenter(), 18, 160.f, 0.35f, 5.f,
                                   sf::Color(255, 200, 80), sf::Color(120, 40, 0, 0), &back, 2.6f);
    }
}

void Fire::update(Game* game, float elapsed) {
    // Movement is integrated by MovementSystem; TTL by TTLSystem.
    Entity::update(game, elapsed);

    // Origin is the centre, so place the sprite at the hitbox centre.
    const sf::Vector2f size = getSpriteSize();
    const sf::Vector2f pos = getPosition();
    sprite.setPosition(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);

    if (game) {
        const sf::Vector2f back = -direction;
        game->getParticles().burst(getCenter(), 2, 40.f, 0.3f, 5.f,
                                   sf::Color(255, 170, 60, 220), sf::Color(200, 30, 0, 0), &back, 1.2f);
    }
}
