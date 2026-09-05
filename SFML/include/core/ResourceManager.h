#pragma once
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <unordered_map>

// Process-wide cache for heavyweight, immutable GPU/file resources.
//
// Every entity used to own its own sf::Texture copy, so spawning 30 mushrooms
// uploaded the same PNG 30 times. Resources returned here live until clear()
// (or program exit); returned references stay valid across later insertions
// because std::unordered_map never relocates its nodes.
class ResourceManager {
public:
    static ResourceManager& get();

    // Loads on first use, throws std::runtime_error if the file is missing.
    const sf::Texture& texture(const std::string& file);
    const sf::Font& font(const std::string& file);

    bool hasTexture(const std::string& file) const { return textures.count(file) != 0; }
    size_t textureCount() const { return textures.size(); }
    size_t fontCount() const { return fonts.size(); }

    // Drops everything. Only safe when no sprite still references a texture.
    void clear();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

private:
    ResourceManager() = default;

    std::unordered_map<std::string, sf::Texture> textures;
    std::unordered_map<std::string, sf::Font> fonts;
};
