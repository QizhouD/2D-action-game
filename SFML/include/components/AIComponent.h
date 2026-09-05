#pragma once
#include <SFML/System/Vector2.hpp>
#include "Components.h"

// Behaviour state for AISystem. Pure data; the system owns the logic.
class AIComponent : public Component {
public:
    enum class State { Patrol, Chase };

    ComponentID getID() const override { return ComponentID::AI; }

    AIComponent(float chaseRangePx, float patrolMinSec = 0.8f, float patrolMaxSec = 2.2f)
        : chaseRange(chaseRangePx), patrolMin(patrolMinSec), patrolMax(patrolMaxSec) {}

    State state = State::Patrol;
    sf::Vector2f direction{ 0.f, 0.f }; // unit vector, (0,0) = standing still
    float retargetTimer = 0.f;          // seconds until a new patrol direction is picked
    bool hitWall = false;               // set by MovementSystem, consumed by AISystem
    float stuckTimer = 0.f;             // how long a chase has been blocked by a wall
    float ignorePlayerTimer = 0.f;      // after giving up a chase, wander for a while

    float chaseRange;
    float patrolMin;
    float patrolMax;
};
