#pragma once
#include "Entity.h"
#include "../../include/core/Balance.h"
#include <string>

// Pickups never move; their bounding box is fixed at init/setPosition time.
// Amounts come from Balance::get().pickup.

class Potion : public Entity {
public:
    Potion() : Entity(EntityType::POTION) {}
    ~Potion() {}

    void update(Game*, float = 1.0f) override {}
    bool blocksOnWalls() const override { return false; }
    int getHealth() const { return Balance::get().pickup.potionHeal; }
};

class Log : public Entity {
public:
    Log() : Entity(EntityType::LOG) {}
    ~Log() {}

    void update(Game*, float = 1.0f) override {}
    bool blocksOnWalls() const override { return false; }
    int getWood() const { return Balance::get().pickup.logWood; }
};
