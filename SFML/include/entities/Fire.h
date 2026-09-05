#pragma once
#include "Entity.h"
#include <memory>
#include "../../include/components/TTLComponent.h"
#include "../../include/components/VelocityComponent.h"

class Fire : public Entity {
public:
    // Lifetime in simulation ticks (60 Hz) and travel speed in pixels per second.
    static const int   startTimeToLive = 150;
    static const float speed;

    Fire();
    ~Fire();

    void update(Game* game, float elapsed = 1.0f) override;

    int getTTL() const { return ttl ? ttl->getTTL() : 0; }
    std::shared_ptr<VelocityComponent> getVelocityComp() const { return velocity; }
    std::shared_ptr<TTLComponent> getTTLComponent() const override;

private:
    std::shared_ptr<TTLComponent> ttl;
    std::shared_ptr<VelocityComponent> velocity;
};
