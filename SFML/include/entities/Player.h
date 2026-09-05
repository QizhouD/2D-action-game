#pragma once
#include "Entity.h"
#include "../../include/components/HealthComponent.h"
#include "../../include/components/VelocityComponent.h"
#include <memory>
#include <set>
#include "../../include/utils/Observer.h"

class InputComponent;
class Fire;

// Tuning values (speed, HP, axe damage, ...) come from Balance::get().player.
class Player : public Entity {
public:
    Player();
    ~Player() override;

    // Body contact (called by Game when hitboxes overlap).
    void handlePotionCollision(Entity* potion);
    void handleEnemyCollision(Entity* enemy);

    // Axe: the swing has its own reach in front of the dwarf.
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
    // If the dwarf is mid-action the request is buffered for a short window
    // and fired as soon as it is free, so mashed inputs are not dropped.
    void startAttack();
    void startShout();

    // Last movement direction (4-way), used for aiming fireballs.
    const sf::Vector2f& getFacing() const { return facing; }

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

    enum class Buffered { None, Attack, Shout };

    bool attacking;
    bool shouting;
    bool fireSpawnedThisShout;
    bool dead;
    float invulnTimer;
    Buffered buffered = Buffered::None;
    float bufferTimer = 0.f;
    sf::Vector2f facing{ 1.f, 0.f };
    std::set<EntityID> hitThisSwing;   // enemies already damaged by the current attack
    std::shared_ptr<HealthComponent> healthComp;
    int wood;
    float shootCooldown;
    std::shared_ptr<InputComponent> input;
    std::shared_ptr<VelocityComponent> velocity;
};
