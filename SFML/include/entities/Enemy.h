#pragma once
#include "Entity.h"
#include "../../include/components/HealthComponent.h"
#include "../../include/components/VelocityComponent.h"
#include "../../include/components/AIComponent.h"
#include "../../include/core/Balance.h"   // EnemyStats
#include <memory>
#include <string>

class Enemy : public Entity {
public:
    explicit Enemy(const EnemyStats& stats);
    ~Enemy() override = default;

    void init(const std::string& textureFile, float scale) override;
    void update(Game* game, float elapsed) override;
    void draw(Window* window) override;
    void onWallHit(Game* game) override;

    // Applies damage; returns true if this hit killed the enemy.
    bool takeDamage(int amount);
    bool isDying() const { return dying; }
    bool isAlive() const { return !dying && !isDeleted(); }
    // Score/drop bookkeeping: the Game credits each death exactly once.
    bool isCredited() const { return credited; }
    void markCredited() { credited = true; }

    const EnemyStats& getStats() const { return stats; }
    int getContactDamage() const { return stats.contactDamage; }
    std::shared_ptr<HealthComponent> getHealthComp() const { return health; }

private:
    EnemyStats stats;
    std::shared_ptr<HealthComponent> health;
    std::shared_ptr<VelocityComponent> velocity;
    std::shared_ptr<AIComponent> ai;

    float hitFlash = 0.f;     // seconds left of red tint after a hit
    bool  dying = false;
    bool  credited = false;
    float deathTimer = 0.f;   // shrink/fade duration
    static constexpr float deathDuration = 0.35f;
    static constexpr float hitFlashDuration = 0.15f;
};
