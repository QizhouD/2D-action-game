#pragma once
#include "Entity.h"
#include "../../include/components/HealthComponent.h"
#include "../../include/components/VelocityComponent.h"
#include <memory>
#include <set>
#include "../../include/utils/Observer.h"

class InputComponent;
class Fire;

class Player : public Entity {
public:
    static const int startingHealth = 80;
    static const int maxHealth = 100;
    static const int maxWood = 999;
    static const float playerSpeed;       // pixels per second
    static const int   shootingCost;      // wood per fireball
    static const float shootCooldownTime; // seconds between fireballs
    static const int   axeDamage;         // per hit on an enemy
    static const float invulnerableTime;  // seconds of i-frames after being hit

    Player();
    ~Player() override;

    // Body contact (called by Game when hitboxes overlap).
    void handlePotionCollision(Entity* potion);
    void handleEnemyCollision(Entity* enemy);

    // Axe: the swing has its own reach in front of the dwarf.
    static const float axeReach;              // px beyond the body hitbox
    bool isSwingActive() const;               // attack animation on its action frames
    Rectangle getAttackBox() const;
    void chopLog(Entity* log);                // once per log
    void hitEnemy(Entity* enemy);             // once per enemy per swing

    // Damage / death
    bool takeDamage(int amount);              // returns true if damage was applied
    bool isInvulnerable() const { return invulnTimer > 0.f; }
    bool isDead() const { return dead; }
    bool isDeathAnimationDone() const;

    // Reset for a new game (health, wood, flags). Position is set separately.
    void resetStats();
    // Called when a new level starts: clears transient state, keeps HP/wood.
    void onLevelStart();

    // Overridden initialization functions.
    void init(const std::string& textureFile, float scale) override;
    void initSpriteSheet(const std::string& spriteSheetFile) override;
    // Update and draw functions.
    void update(Game* game, float elapsed) override;
    void draw(Window* window) override;

    // Actions (called by commands). Each starts its animation exactly once.
    void startAttack();
    void startShout();

    bool isAttacking() const { return attacking; }
    bool isShouting() const { return shouting; }
    bool isBusy() const { return attacking || shouting; }

    std::shared_ptr<HealthComponent> getHealthComp() const { return healthComp; }
    int getWood() const { return wood; }
    void addWood(int w);

    // Return velocity component pointer.
    std::shared_ptr<VelocityComponent> getVelocityComp() const { return velocity; }

    // Position the sprite in the tile map.
    void positionSprite(int row, int col, int spriteWH, float tileScale);

    void setObserver(std::shared_ptr<Observer> obs);
    std::shared_ptr<Observer> getObserver() const;

private:
    std::shared_ptr<Fire> createFire() const;
    void updateMovementAnimation();
    std::shared_ptr<Observer> observer;

    bool attacking;
    bool shouting;
    bool fireSpawnedThisShout;
    bool dead;
    float invulnTimer;
    std::set<EntityID> hitThisSwing;   // enemies already damaged by the current attack
    std::shared_ptr<HealthComponent> healthComp;
    int wood;
    float shootCooldown;
    std::shared_ptr<InputComponent> input;
    std::shared_ptr<VelocityComponent> velocity;
};
