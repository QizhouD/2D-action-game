#include "../../include/graphics/ParticleSystem.h"
#include "../../include/graphics/Window.h"
#include <cmath>
#include <random>

namespace {
    std::mt19937& rng() {
        static std::mt19937 gen{ 987654u };
        return gen;
    }
    float frand(float a, float b) {
        std::uniform_real_distribution<float> d(a, b);
        return d(rng());
    }
    sf::Color lerp(const sf::Color& a, const sf::Color& b, float t) {
        auto mix = [t](sf::Uint8 x, sf::Uint8 y) { return static_cast<sf::Uint8>(x + (y - x) * t); };
        return { mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a) };
    }
}

void ParticleSystem::burst(const sf::Vector2f& pos, int count, float speed, float life, float size,
                           sf::Color color, sf::Color endColor, const sf::Vector2f* dir, float spreadRad)
{
    const float baseAngle = dir ? std::atan2(dir->y, dir->x) : 0.f;
    for (int i = 0; i < count && particles.size() < maxParticles; ++i) {
        const float a = baseAngle + frand(-spreadRad * 0.5f, spreadRad * 0.5f);
        const float s = speed * frand(0.5f, 1.2f);
        Particle p;
        p.pos = pos + sf::Vector2f(frand(-size, size), frand(-size, size));
        p.vel = { std::cos(a) * s, std::sin(a) * s };
        p.maxLife = p.life = life * frand(0.7f, 1.3f);
        p.size = size * frand(0.6f, 1.2f);
        p.color = color;
        p.drag = 0.15f;
        particles.push_back(p);
        endColors.push_back(endColor);
    }
}

void ParticleSystem::update(float elapsed)
{
    for (size_t i = 0; i < particles.size();) {
        Particle& p = particles[i];
        p.life -= elapsed;
        if (p.life <= 0.f) {
            particles[i] = particles.back();
            endColors[i] = endColors.back();
            particles.pop_back();
            endColors.pop_back();
            continue;
        }
        const float keep = std::pow(p.drag, elapsed);
        p.vel *= keep;
        p.pos += p.vel * elapsed;
        ++i;
    }
}

void ParticleSystem::draw(Window& window) const
{
    if (particles.empty()) return;
    sf::VertexArray quads(sf::Quads, particles.size() * 4);
    for (size_t i = 0; i < particles.size(); ++i) {
        const Particle& p = particles[i];
        const float t = 1.f - p.life / p.maxLife;           // 0 fresh .. 1 dead
        const sf::Color c = lerp(p.color, endColors[i], t);
        const float s = p.size * (1.f - 0.6f * t);
        quads[i * 4 + 0] = sf::Vertex({ p.pos.x - s, p.pos.y - s }, c);
        quads[i * 4 + 1] = sf::Vertex({ p.pos.x + s, p.pos.y - s }, c);
        quads[i * 4 + 2] = sf::Vertex({ p.pos.x + s, p.pos.y + s }, c);
        quads[i * 4 + 3] = sf::Vertex({ p.pos.x - s, p.pos.y + s }, c);
    }
    window.draw(quads);
}
