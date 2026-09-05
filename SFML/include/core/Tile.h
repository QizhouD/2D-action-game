#pragma once
#include "../../include/utils/Vector2.h"
#include <SFML/Graphics.hpp>

enum class TileType { CORRIDOR, WALL, EXIT };

class Tile {
public:
    Tile(TileType t);

    // `tex` must outlive the tile; Board obtains it from ResourceManager.
    void loadTile(int x, int y, float scale, const sf::Texture& tex);
    void draw(class Window* window);
    TileType getType() const { return type; }
    bool isWalkable() const { return type != TileType::WALL; }
    sf::Vector2f getWorldPosition() const { return sprite.getPosition(); }
    float getWorldSize() const { return worldSize; }

private:
    TileType type;
    sf::Vector2i position;
    sf::Sprite sprite;
    float worldSize = 0.f;
};
