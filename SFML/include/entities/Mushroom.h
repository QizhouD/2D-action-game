#pragma once
#include "Entity.h"
#include "../components/HealthComponent.h"
#include "../components/VelocityComponent.h"
#include "../components/ColliderComponent.h"
#include <memory>

class Mushroom : public Entity {
public:
    static constexpr int defaultHealth = 30;
    static constexpr float moveSpeed = 50.f; // pixels per second

    Mushroom();
    ~Mushroom() override = default;

    void init(const std::string& textureFile, float scale) override;
    void update(Game* game, float elapsed) override;

    std::shared_ptr<HealthComponent> getHealthComp() const { return health; }
    std::shared_ptr<VelocityComponent> getVelocityComp() const { return velocity; }

private:
    std::shared_ptr<HealthComponent> health;
    std::shared_ptr<VelocityComponent> velocity;
    std::shared_ptr<ColliderComponent> collider;
};