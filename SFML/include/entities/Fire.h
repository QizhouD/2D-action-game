#pragma once
#include "Entity.h"
#include <memory>
#include "../../include/components/TTLComponent.h"
#include "../../include/components/VelocityComponent.h"

// Speed / lifetime / damage come from Balance::get().fire.
class Fire : public Entity {
public:
    Fire();
    ~Fire();

    void init(const std::string& textureFile, float scale) override;
    void update(Game* game, float elapsed = 1.0f) override;
    // Fireballs burst on walls.
    void onWallHit(Game* game) override;

    // Unit direction of travel; also rotates the sprite.
    void setDirection(const sf::Vector2f& dir);
    const sf::Vector2f& getDirection() const { return direction; }

    int getDamage() const { return damage; }
    void setDamage(int d) { damage = d; }

    int getTTL() const { return ttl ? ttl->getTTL() : 0; }
    std::shared_ptr<VelocityComponent> getVelocityComp() const { return velocity; }
    std::shared_ptr<TTLComponent> getTTLComponent() const override;

private:
    int damage;
    sf::Vector2f direction{ 1.f, 0.f };
    std::shared_ptr<TTLComponent> ttl;
    std::shared_ptr<VelocityComponent> velocity;
};
