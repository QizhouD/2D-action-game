#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Window;

// Minimal CPU particle system drawn as one quad batch. Used for fireball
// trails, hit sparks and death puffs.
class ParticleSystem {
public:
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life;      // seconds left
        float maxLife;
        float size;      // half extent in px
        sf::Color color;
        float drag;      // 0..1 velocity kept per second (1 = none)
    };

    // Emits `count` particles around `pos` with velocities of roughly `speed`
    // in random directions (spread = full circle when no `dir` is given).
    void burst(const sf::Vector2f& pos, int count, float speed, float life, float size,
               sf::Color color, sf::Color endColor, const sf::Vector2f* dir = nullptr, float spreadRad = 6.2832f);

    void update(float elapsed);
    void draw(Window& window) const;
    void clear() { particles.clear(); }
    size_t count() const { return particles.size(); }

private:
    std::vector<Particle> particles;
    std::vector<sf::Color> endColors;   // parallel to particles
    static constexpr size_t maxParticles = 4000;
};
