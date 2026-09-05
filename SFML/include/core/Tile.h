#pragma once
#include "../../include/utils/Vector2.h"
#include <SFML/Graphics.hpp>
#include <memory>

class TileTexture;

enum class TileType { CORRIDOR, WALL, EXIT };

class Tile {
public:
    Tile(TileType t);

    void loadTile(int x, int y, float scale, std::shared_ptr<TileTexture> sharedTex);
    void draw(class Window* window);
    TileType getType() const { return type; }
    bool isWalkable() const { return type != TileType::WALL; }
    sf::Vector2f getWorldPosition() const { return sprite.getPosition(); }
    float getWorldSize() const { return worldSize; }

private:
    TileType type;
    sf::Vector2i position;
    sf::Sprite sprite;
    std::shared_ptr<TileTexture> texture; //Flyweight: shared texture
    float worldSize = 0.f;
};
