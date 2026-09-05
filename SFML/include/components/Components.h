#pragma once

// Each Component type has a unique ID; Entity keeps a Bitmask of the IDs it
// owns so Systems can cheaply test "does this entity have what I need".
enum class ComponentID
{
    UNDEFINED = -1,
    INPUT = 0,
    POSITION,
    VELOCITY,
    HEALTH,
    TTL,
    AI,
    COUNT
};

class Component
{
public:
    virtual ~Component() = default;
    virtual ComponentID getID() const = 0;
};
