#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <SFML/Graphics.hpp>
#include "Tile.h"
#include "../../include/graphics/TileTexture.h"

class Rectangle;

class Board {
public:
    Board(size_t width, size_t height, float tileWorldSize);
    ~Board();

    void addTile(int x, int y, float scale, TileType type, const std::string& textureFile);
    void draw(class Window* wnd);
    bool inBounds(int x, int y) const;

    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }
    float getTileSize() const { return tileSize; }
    sf::Vector2f getWorldSize() const { return { width * tileSize, height * tileSize }; }

    // --- Collision queries (world coordinates) ---------------------------
    const Tile* tileAtWorld(float wx, float wy) const;
    bool isWalkable(float wx, float wy) const;
    // True if all four corners of the box are on walkable tiles.
    bool isBoxWalkable(const Rectangle& box) const;

    // --- Exit tile --------------------------------------------------------
    bool hasExit() const { return exitTile != nullptr; }
    void setExitActive(bool on) { exitActive = on; }
    bool isExitActive() const { return exitActive; }
    // True when the box's centre is over the (active) exit tile.
    bool isOnActiveExit(const Rectangle& box) const;

    void advanceAnimation(float elapsed) { animTime += elapsed; }

private:
    size_t width, height;
    float tileSize;
    std::vector<Tile*> grid;
    Tile* exitTile = nullptr;
    bool exitActive = false;
    float animTime = 0.f;

    // Flyweight storage
    std::unordered_map<std::string, std::shared_ptr<TileTexture>> textureMap;
};
