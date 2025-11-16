#pragma once
#include "../../include/utils/Vector2.h"
#include <SFML/Graphics.hpp>
#include <memory>

class TileTexture;

// Treat WALL as solid for collisions, CORRIDOR as non-solid by default
enum class TileType { CORRIDOR, WALL };

class Tile {
public:
    Tile(TileType t);

    void loadTile(int x, int y, float scale, std::shared_ptr<TileTexture> sharedTex);
    void draw(class Window* window);
    TileType getType() const { return type; }

    // Collision helpers
    bool isSolid() const { return type == TileType::WALL; }
    // World-space bounds of this tile's sprite (used for AABB checks)
    sf::FloatRect getBounds() const { return sprite.getGlobalBounds(); }

private:
    TileType type;
    sf::Vector2i position;
    sf::Sprite sprite;
    std::shared_ptr<TileTexture> texture; //Flyweight: shared texture
};
