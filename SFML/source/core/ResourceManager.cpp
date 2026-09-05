#include "../../include/core/ResourceManager.h"
#include <stdexcept>

ResourceManager& ResourceManager::get()
{
    static ResourceManager instance;
    return instance;
}

// sf::Texture / sf::Font are not movable in SFML 2.x, so load in place.
const sf::Texture& ResourceManager::texture(const std::string& file)
{
    auto it = textures.find(file);
    if (it != textures.end()) return it->second;

    sf::Texture& tex = textures[file];
    if (!tex.loadFromFile(file)) {
        textures.erase(file);
        throw std::runtime_error("Texture not found: " + file);
    }
    return tex;
}

const sf::Font& ResourceManager::font(const std::string& file)
{
    auto it = fonts.find(file);
    if (it != fonts.end()) return it->second;

    sf::Font& f = fonts[file];
    if (!f.loadFromFile(file)) {
        fonts.erase(file);
        throw std::runtime_error("Font not found: " + file);
    }
    return f;
}

void ResourceManager::clear()
{
    textures.clear();
    fonts.clear();
}
