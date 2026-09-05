#include "../../include/systems/Systems.h"
#include "../../include/components/AIComponent.h"
#include "../../include/components/VelocityComponent.h"
#include "../../include/entities/Entity.h"
#include "../../include/entities/Player.h"
#include "../../include/core/Game.h"
#include <cmath>
#include <random>

namespace {
    std::mt19937& rng() {
        static std::mt19937 gen{ std::random_device{}() };
        return gen;
    }

    sf::Vector2f randomCardinal(const sf::Vector2f& avoid) {
        static const sf::Vector2f dirs[4] = { { 1.f, 0.f }, { -1.f, 0.f }, { 0.f, 1.f }, { 0.f, -1.f } };
        std::uniform_int_distribution<int> pick(0, 3);
        sf::Vector2f d = dirs[pick(rng())];
        // Never pick the direction we just bounced off.
        if (d == avoid) d = dirs[(pick(rng()) + 1) % 4];
        return d;
    }

    float length(const sf::Vector2f& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
}

AISystem::AISystem() {
    componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::AI));
    componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::VELOCITY));
}

void AISystem::update(Game* game, Entity* entity, float elapsed) {
    auto ai  = std::dynamic_pointer_cast<AIComponent>(entity->getComponent(ComponentID::AI));
    auto vel = std::dynamic_pointer_cast<VelocityComponent>(entity->getComponent(ComponentID::VELOCITY));
    if (!ai || !vel) return;

    auto player = game->getPlayer();
    if (!player || player->isDead()) {
        vel->setVelocity(0.f, 0.f);
        return;
    }

    const sf::Vector2f toPlayer = player->getCenter() - entity->getCenter();
    const float dist = length(toPlayer);

    if (ai->ignorePlayerTimer > 0.f) ai->ignorePlayerTimer -= elapsed;

    if (dist <= ai->chaseRange && ai->ignorePlayerTimer <= 0.f) {
        ai->state = AIComponent::State::Chase;
        ai->direction = (dist > 0.001f) ? toPlayer / dist : sf::Vector2f(0.f, 0.f);

        // Sliding along walls is handled by MovementSystem's per-axis moves;
        // if we are still blocked after a while there is no straight path, so
        // give up for a bit and wander instead of hugging the wall forever.
        if (ai->hitWall) {
            ai->stuckTimer += elapsed;
            ai->hitWall = false;
            if (ai->stuckTimer > 0.6f) {
                ai->stuckTimer = 0.f;
                ai->ignorePlayerTimer = 1.5f;
                ai->state = AIComponent::State::Patrol;
                ai->retargetTimer = 0.f;
            }
        }
        else {
            ai->stuckTimer = 0.f;
        }
    }
    else {
        if (ai->state == AIComponent::State::Chase) {
            // Lost the player: resume patrol immediately.
            ai->state = AIComponent::State::Patrol;
            ai->retargetTimer = 0.f;
            ai->stuckTimer = 0.f;
        }
        ai->retargetTimer -= elapsed;
        if (ai->retargetTimer <= 0.f || ai->hitWall) {
            std::uniform_real_distribution<float> t(ai->patrolMin, ai->patrolMax);
            ai->direction = randomCardinal(ai->hitWall ? ai->direction : sf::Vector2f(0.f, 0.f));
            ai->retargetTimer = t(rng());
            ai->hitWall = false;
        }
    }

    vel->setVelocity(ai->direction.x, ai->direction.y);
}
