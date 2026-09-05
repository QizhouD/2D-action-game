#include "../../include/entities/Fire.h"
#include "../../include/components/TTLComponent.h"
#include "../../include/components/VelocityComponent.h"
#include "../../include/core/Game.h"
#include <iostream>

const float Fire::speed = 200.f;

Fire::Fire() : Entity(EntityType::FIRE) {
    ttl = std::make_shared<TTLComponent>(startTimeToLive);
    addComponent(ttl);

    // Direction is set by the spawner; the speed factor turns it into px/s.
    velocity = std::make_shared<VelocityComponent>(speed);
    addComponent(velocity);
}

Fire::~Fire() {}

std::shared_ptr<TTLComponent> Fire::getTTLComponent() const {
    return ttl;
}

void Fire::update(Game* game, float elapsed) {
    // Movement is integrated by MovementSystem; TTL by TTLSystem.
    Entity::update(game, elapsed);
}
