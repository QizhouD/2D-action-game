#include "../../include/core/Board.h"
#include <stdexcept>
#include <iostream>
#include <cmath>

Board::Board(size_t w, size_t h) : width(w), height(h) {
    grid.resize(width * height, nullptr);
}

Board::~Board() {
    for (auto* tile : grid) delete tile;
}

bool Board::inBounds(int x, int y) const {
    return x >= 0 && x < static_cast<int>(width) && y >= 0 && y < static_cast<int>(height);
}

void Board::addTile(int x, int y, float scale, TileType type, const std::string& textureFile) {
    if (!inBounds(x, y)) throw std::runtime_error("addTile: out of bounds");

    int idx = y * static_cast<int>(width) + x;
    if (grid[idx]) {
        delete grid[idx];
        grid[idx] = nullptr;
    }

    // Reuse or load texture
    auto it = textureMap.find(textureFile);
    std::shared_ptr<TileTexture> tex;

    if (it != textureMap.end()) {
        tex = it->second;
    }
    else {
        tex = std::make_shared<TileTexture>();
        if (!tex->loadFromFile(textureFile)) {
            throw std::runtime_error("Texture load failed: " + textureFile);
        }
        textureMap[textureFile] = tex;
    }

    Tile* newTile = new Tile(type);
    newTile->loadTile(x, y, scale, tex);
    grid[idx] = newTile;
}

void Board::draw(Window* wnd) {
    for (int y = 0; y < static_cast<int>(height); y++) {
        for (int x = 0; x < static_cast<int>(width); x++) {
            Tile* tile = grid[y * static_cast<int>(width) + x];
            if (tile) tile->draw(wnd);
        }
    }
}

// ---- Collision helpers ----
Tile* Board::getTile(int gx, int gy) const {
    if (!inBounds(gx, gy)) return nullptr;
    return grid[gy * static_cast<int>(width) + gx];
}

bool Board::isSolid(int gx, int gy) const {
    Tile* t = getTile(gx, gy);
    return t ? t->isSolid() : false;
}

sf::FloatRect Board::tileBounds(int gx, int gy) const {
    Tile* t = getTile(gx, gy);
    return t ? t->getBounds() : sf::FloatRect();
}

int Board::worldToGrid(float w, float tileSizePx) const {
    if (tileSizePx <= 0.f) return 0;
    return static_cast<int>(std::floor(w / tileSizePx));
}
