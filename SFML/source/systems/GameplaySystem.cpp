#include "../../include/systems/Systems.h"
#include "../../include/entities/Entity.h"
#include "../../include/entities/StaticEntities.h"
#include "../../include/entities/Player.h"
#include "../../include/entities/Fire.h"
#include "../../include/entities/Mushroom.h"
#include "../../include/utils/Rectangle.h"
#include "../../include/core/Game.h"
#include "../../include/core/ServiceLocator.h"
#include <iostream>

GameplaySystem::GameplaySystem() {
    // Run for all world entities that have a position (i.e., nearly all entities)
    componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::POSITION));
}

void GameplaySystem::update(Game* game, Entity* entity, float elapsed) {
    static float mushDamageCooldown = 0.f;
    // Tick cooldown timer
    if (mushDamageCooldown > 0.f) mushDamageCooldown -= elapsed;
    if (entity->getEntityType() == EntityType::PLAYER)
        return;

    // Get the player entity.
    std::shared_ptr<Player> player = game->getPlayer();
    if (!player)
        return;

    // Retrieve bounding boxes.
    Rectangle& playerBB = player->getBoundingBox();
    Rectangle& entityBB = entity->getBoundingBox();

    // Check if the player and this entity collide.
    if (playerBB.intersects(entityBB)) {
        switch (entity->getEntityType()) {
        case EntityType::POTION:
        {
            Potion* pot = dynamic_cast<Potion*>(entity);
            if (pot) {
                int potionHealth = pot->getHealth();
                player->getHealthComp()->changeHealth(potionHealth);
                entity->deleteEntity();
                if (player->getObserver()) {
                    player->getObserver()->onPotionCollected();
                    ServiceLocator::getAudio()->playSound("pickup");
                }
            }
            break;
        }
        case EntityType::LOG:
        {
            // Defer log collection to Player::handleLogCollision via Game::collisionCallbacks.
            // That handler enforces the attack animation's action window, preventing premature deletion.
            break;
        }
        case EntityType::MUSHROOM:
        {
            // Damage player on contact with cooldown to reduce DPS
            const float mushHitInterval = 0.5f; // seconds between contact damage ticks
            if (mushDamageCooldown <= 0.f) {
                player->getHealthComp()->changeHealth(-1);
                mushDamageCooldown = mushHitInterval;
            }
            
            // Player melee attack damages mushrooms
            Mushroom* mush = dynamic_cast<Mushroom*>(entity);
            if (mush && player->isAttacking()) {
                mush->getHealthComp()->changeHealth(-10);
                if (mush->getHealthComp()->getHealth() <= 0) {
                    game->incrementKills();
                    entity->deleteEntity();
                }
            }
            break;
        }
        default:
            break;
        }
    }

    // Fire projectile vs Mushroom collisions
    if (entity->getEntityType() == EntityType::MUSHROOM) {
        Mushroom* mush = dynamic_cast<Mushroom*>(entity);
        if (mush) {
            for (const auto& e : game->getEntities()) {
                if (!e || e->isDeleted()) { continue; }
                if (e->getEntityType() == EntityType::FIRE) {
                    if (e->getBoundingBox().intersects(entity->getBoundingBox())) {
                        // Damage mushroom and remove fire
                        mush->getHealthComp()->changeHealth(-15);
                        e->deleteEntity();
                        if (mush->getHealthComp()->getHealth() <= 0) {
                            game->incrementKills();
                            entity->deleteEntity();
                        }
                        break; // handle one fire per frame
                    }
                }
            }
        }
    }

    // Player reached goal -> either show settlement (lvl2) or load next level
    if (entity->getEntityType() == EntityType::GOAL) {
        if (playerBB.intersects(entityBB)) {
            if (game->getCurrentLevelIndex() == 2) {
                game->triggerSettlement();
            } else {
                game->loadNextLevel();
            }
        }
    }
}
