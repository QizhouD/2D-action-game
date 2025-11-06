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
    // Optional: If required, set a component mask.
}

void GameplaySystem::update(Game* game, Entity* entity, float) {
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
            Log* logEntity = dynamic_cast<Log*>(entity);
            if (logEntity && player->isAttacking()) {
                int woodCollected = logEntity->getWood();
                player->addWood(woodCollected);
                entity->deleteEntity();
            }
            break;
        }
        case EntityType::MUSHROOM:
        {
            // Damage player on contact
            player->getHealthComp()->changeHealth(-1);
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
                if (e->getEntityType() == EntityType::FIRE) {
                    if (e->getBoundingBox().intersects(entity->getBoundingBox())) {
                        // Damage mushroom and remove fire
                        mush->getHealthComp()->changeHealth(-10);
                        e->deleteEntity();
                        if (mush->getHealthComp()->getHealth() <= 0) {
                            entity->deleteEntity();
                        }
                        break; // handle one fire per frame
                    }
                }
            }
        }
    }

    // Player reached goal -> load next level
    if (entity->getEntityType() == EntityType::GOAL) {
        if (playerBB.intersects(entityBB)) {
            game->loadNextLevel();
        }
    }
}
