#include "../../include/core/Tile.h"
#include "../../include/graphics/Window.h"

Tile::Tile(TileType t) : type(t) {}

void Tile::loadTile(int x, int y, float sc, const sf::Texture& tex) {
    position.x = x;
    position.y = y;

    sprite.setTexture(tex, true);
    sprite.setScale(sc, sc);

    const sf::Vector2u texSize = tex.getSize();
    worldSize = texSize.x * sc;
    sprite.setPosition(static_cast<float>(x) * texSize.x * sc,
                       static_cast<float>(y) * texSize.y * sc);
}

void Tile::draw(Window* window) {
    window->draw(sprite);
}
