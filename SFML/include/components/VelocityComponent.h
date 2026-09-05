#pragma once
#include <SFML/System/Vector2.hpp>
#include "Components.h"
class PositionComponent;

class VelocityComponent: public Component{
public:
    ComponentID getID() const override {
        return ComponentID::VELOCITY;
    }

    VelocityComponent(float spd = 1.f);
    void setVelocity(float x, float y);
    const sf::Vector2f& getVelocity() const;
    float getSpeed() const { return speed; }
    void setSpeed(float s) { speed = s; }
    // velocity * speed, in pixels per second.
    sf::Vector2f getWorldVelocity() const { return { velocity.x * speed, velocity.y * speed }; }
    // Update the PositionComponent based on velocity * speed * elapsed.
    void update(PositionComponent& posComp, float elapsed);
private:
    sf::Vector2f velocity;
    float speed;
};
