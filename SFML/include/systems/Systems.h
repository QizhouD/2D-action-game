#pragma once
#include "../../include/core/Game.h"
#include "../../include/entities/Entity.h"
#include "../../include/utils/Bitmask.h"

// Abstract base class for all systems
class System {
protected:
    Bitmask componentMask; // Bitmask indicating which components this system operates on

public:
    virtual ~System() = default;

    // Pure virtual update function
    virtual void update(Game* game, Entity* entity, float elapsed) = 0;

    // Check if an entity matches the system's required component mask
    bool validate(Entity* entity) const {
        if (componentMask.getMask() == 0) return false;
        return entity->hasComponent(componentMask);
    }
};

class TTLSystem : public System {
public:
    TTLSystem();
    void update(Game* game, Entity* entity, float elapsed) override;
};

class InputSystem : public System {
public:
    InputSystem(); 
    void update(Game* game, Entity* entity, float elapsed) override;
};

// Integrates velocity into position. Entities that blocksOnWalls() are moved
// one axis at a time and stopped (sliding along) at WALL tiles / map edges.
class MovementSystem : public System {
public:
    MovementSystem();
    void update(Game* game, Entity* entity, float elapsed) override;
};

// Drives entities with an AIComponent: patrol randomly, chase the player
// when in range. Writes a unit direction into the VelocityComponent.
class AISystem : public System {
public:
    AISystem();
    void update(Game* game, Entity* entity, float elapsed) override;
};