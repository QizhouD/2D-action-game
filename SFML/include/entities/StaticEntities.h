#pragma once
#include "Entity.h"
#include <string>

// Pickups never move; their bounding box is fixed at init/setPosition time.

class Potion : public Entity {
public:
    Potion() : Entity(EntityType::POTION) {}
    ~Potion() {}

    void update(Game*, float = 1.0f) override {}
    bool blocksOnWalls() const override { return false; }
    int getHealth() const { return potionHealth; }
protected:
    const int potionHealth = 10;
};

class Log : public Entity {
public:
    Log() : Entity(EntityType::LOG) {}
    ~Log() {}

    void update(Game*, float = 1.0f) override {}
    bool blocksOnWalls() const override { return false; }
    int getWood() const { return woodAdded; }
protected:
    const int woodAdded = 15;
};
