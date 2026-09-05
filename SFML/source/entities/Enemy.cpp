#include "../../include/entities/Enemy.h"
#include "../../include/core/Game.h"
#include <algorithm>

Enemy::Enemy(const EnemyStats& st)
    : Entity(EntityType::ENEMY), stats(st)
{
    health = std::make_shared<HealthComponent>(stats.health, stats.health);
    addComponent(health);

    velocity = std::make_shared<VelocityComponent>(stats.speed);
    addComponent(velocity);

    ai = std::make_shared<AIComponent>(stats.chaseRange);
    addComponent(ai);
}

void Enemy::init(const std::string& textureFile, float scale) {
    Entity::init(textureFile, scale);
    // Hitbox a bit smaller than the sprite so enemies fit through 1-tile corridors.
    const sf::Vector2f size = getSpriteSize();
    const float inset = size.x * stats.hitboxInset;
    const float w = size.x - 2.f * inset;
    const float h = size.y - 2.f * inset;
    // Anchor towards the feet: the cap of the mushroom is mostly air.
    setHitbox(inset, size.y - h - inset * 0.5f, w, h);
}

void Enemy::onWallHit(Game* /*game*/) {
    ai->hitWall = true;
}

bool Enemy::takeDamage(int amount) {
    if (dying) return false;
    health->changeHealth(-amount);
    hitFlash = hitFlashDuration;
    if (health->getHealth() <= 0) {
        dying = true;
        deathTimer = deathDuration;
        velocity->setVelocity(0.f, 0.f);
        velocity->setSpeed(0.f);
        return true;
    }
    return false;
}

void Enemy::update(Game* game, float elapsed) {
    if (hitFlash > 0.f) hitFlash -= elapsed;

    if (dying) {
        deathTimer -= elapsed;
        if (deathTimer <= 0.f) {
            deleteEntity();
            return;
        }
        // Shrink towards the centre while fading out.
        const float t = std::max(0.f, deathTimer / deathDuration);
        const sf::Vector2f centre = getCenter();
        const float s = stats.scale * t;
        sprite.setScale(s, s);
        const float half = getTextureSize().x * s * 0.5f;
        sprite.setPosition(centre.x - half, centre.y - half);
        sprite.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255 * t)));
        return;
    }

    // Face the direction of travel.
    const sf::Vector2f v = velocity->getVelocity();
    if (v.x != 0.f) {
        const float sx = (v.x < 0.f) ? -stats.scale : stats.scale;
        sprite.setScale(sx, stats.scale);
        // Flipping around the left edge would shift the sprite; compensate.
        sprite.setOrigin(v.x < 0.f ? static_cast<float>(getTextureSize().x) : 0.f, 0.f);
    }

    sprite.setColor(hitFlash > 0.f ? sf::Color(255, 90, 90) : sf::Color::White);

    Entity::update(game, elapsed);
}

void Enemy::draw(Window* window) {
    Entity::draw(window);
}
