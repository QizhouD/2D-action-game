#include "../../include/core/Board.h"
#include "../../include/graphics/Window.h"
#include "../../include/utils/Rectangle.h"
#include <cmath>
#include <stdexcept>
#include <iostream>

Board::Board(size_t w, size_t h, float tileWorldSize)
    : width(w), height(h), tileSize(tileWorldSize) {
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
        if (grid[idx] == exitTile) exitTile = nullptr;
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
    if (type == TileType::EXIT) exitTile = newTile;
}

const Tile* Board::tileAtWorld(float wx, float wy) const {
    if (wx < 0.f || wy < 0.f) return nullptr;
    const int tx = static_cast<int>(wx / tileSize);
    const int ty = static_cast<int>(wy / tileSize);
    if (!inBounds(tx, ty)) return nullptr;
    return grid[ty * static_cast<int>(width) + tx];
}

bool Board::isWalkable(float wx, float wy) const {
    const Tile* t = tileAtWorld(wx, wy);
    return t != nullptr && t->isWalkable();
}

bool Board::isBoxWalkable(const Rectangle& box) const {
    const auto& tl = box.getTopLeft();
    const auto& br = box.getBottomRight();
    // Shrink by a hair so a box exactly aligned with a tile edge does not
    // sample the neighbouring tile.
    const float eps = 0.01f;
    return isWalkable(tl.x + eps, tl.y + eps)
        && isWalkable(br.x - eps, tl.y + eps)
        && isWalkable(tl.x + eps, br.y - eps)
        && isWalkable(br.x - eps, br.y - eps);
}

bool Board::isOnActiveExit(const Rectangle& box) const {
    if (!exitTile || !exitActive) return false;
    const auto& tl = box.getTopLeft();
    const auto& br = box.getBottomRight();
    const Tile* t = tileAtWorld((tl.x + br.x) * 0.5f, (tl.y + br.y) * 0.5f);
    return t == exitTile;
}

void Board::draw(Window* wnd) {
    for (int y = 0; y < static_cast<int>(height); y++) {
        for (int x = 0; x < static_cast<int>(width); x++) {
            Tile* tile = grid[y * static_cast<int>(width) + x];
            if (tile) tile->draw(wnd);
        }
    }

    // Exit marker: grey ring while locked, pulsing gold once all enemies are gone.
    if (exitTile) {
        const sf::Vector2f p = exitTile->getWorldPosition();
        const float s = exitTile->getWorldSize();
        const float pulse = exitActive ? 0.5f + 0.5f * std::sin(animTime * 4.f) : 0.f;
        const float radius = s * (0.28f + 0.06f * pulse);

        sf::CircleShape ring(radius, 32);
        ring.setOrigin(radius, radius);
        ring.setPosition(p.x + s * 0.5f, p.y + s * 0.5f);
        ring.setFillColor(exitActive ? sf::Color(255, 210, 60, static_cast<sf::Uint8>(90 + 90 * pulse))
                                     : sf::Color(40, 40, 40, 120));
        ring.setOutlineThickness(5.f);
        ring.setOutlineColor(exitActive ? sf::Color(255, 240, 150) : sf::Color(110, 110, 110));
        wnd->draw(ring);
    }
}
