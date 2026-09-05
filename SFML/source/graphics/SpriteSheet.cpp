#include "../../include/graphics/SpriteSheet.h"
#include "../../include/graphics/AnimDirectional.h"
#include "../../include/core/ResourceManager.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

SpriteSheet::SpriteSheet() :
    spriteScale(1.f, 1.f),
    direction(Direction::Right),
    curAnimation(nullptr)
{
}

SpriteSheet::~SpriteSheet() {
    releaseSheet();
}

void SpriteSheet::releaseSheet() {
    curAnimation = nullptr;
    animations.clear();
}

void SpriteSheet::setSpriteSize(const sf::Vector2i& size) {
    spriteSize = size;
}

void SpriteSheet::setSpriteScale(const sf::Vector2f& scale) {
    spriteScale = scale;
    sprite.setScale(spriteScale);
}

void SpriteSheet::setSpritePosition(const sf::Vector2f& pos) {
    sprite.setPosition(pos);
}

void SpriteSheet::setSpriteDirection(const Direction& dir) {
    if (dir == direction)
        return;
    direction = dir;
    if (curAnimation != nullptr)
        curAnimation->cropSprite();
}

void SpriteSheet::cropSprite(const sf::IntRect& rect) {
    sprite.setTextureRect(rect);
}

bool SpriteSheet::loadSheet(const std::string& file) {
    std::ifstream sheet(file);
    if (!sheet.is_open())
        throw std::runtime_error("ERROR: failed loading spritesheet " + file);

    releaseSheet();
    std::string line;
    while (std::getline(sheet, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::stringstream keystream(line);
        std::string type;
        keystream >> type;
        if (type == "Texture") {
            std::string textureFile;
            keystream >> textureFile;
            texture = &ResourceManager::get().texture(textureFile);
            sprite.setTexture(*texture, true);
        }
        else if (type == "Size") {
            keystream >> spriteSize.x >> spriteSize.y;
            setSpriteSize(spriteSize);
        }
        else if (type == "Scale") {
            keystream >> spriteScale.x >> spriteScale.y;
            setSpriteScale(spriteScale);
        }
        else if (type == "AnimationType") {
            keystream >> animType;
        }
        else if (type == "Animation") {
            std::string name;
            keystream >> name;
            if (animations.find(name) != animations.end())
                throw std::runtime_error("Duplicated animation: " + name + " in sprite sheet " + file);
            std::unique_ptr<AnimBase> anim;
            if (animType == "Directional")
                anim = std::make_unique<AnimDirectional>();
            else
                throw std::runtime_error("Unknown animation type: " + animType + " in sprite sheet " + file);

            keystream >> *anim;
            anim->setSpriteSheet(this);
            anim->setName(name);
            anim->reset();
            animations.emplace(name, std::move(anim));
        }
    }
    sheet.close();
    return true;
}

AnimBase* SpriteSheet::getCurrentAnim() const {
    return curAnimation;
}

bool SpriteSheet::setAnimation(const std::string& name, bool play, bool loop, bool restart) {
    auto itr = animations.find(name);
    if (itr == animations.end()) return false;
    if (itr->second.get() == curAnimation && !restart) return false;
    if (curAnimation)
        curAnimation->stop();
    curAnimation = itr->second.get();
    curAnimation->setLooping(loop);
    if (play)
        curAnimation->play();
    curAnimation->cropSprite();
    return true;
}

void SpriteSheet::update(float elapsedTime) {
    if (curAnimation)
        curAnimation->update(elapsedTime);
}

void SpriteSheet::draw(sf::RenderWindow* window) {
    window->draw(sprite);
}
