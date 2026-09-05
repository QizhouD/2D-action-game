#include "../../include/entities/Entity.h"
#include "../../include/graphics/Window.h"
#include "../../include/utils/Rectangle.h"
#include "../../include/components/PositionComponent.h"
#include <SFML/System/Vector2.hpp>
#include "../../include/utils/Vector2.h"  
#include <iostream>
#include <stdexcept>
#include "../../include/utils/Bitmask.h"
#include "../../include/components/TTLComponent.h"

Entity::Entity()
    : Entity(EntityType::UNDEFINED)
{
}

Entity::Entity(EntityType et)
    : type(et), id(0), isSpriteSheet(false), deleted(false)
{
    // Register the position component so systems that require POSITION
    // (e.g. MovementSystem) actually match this entity.
    positionComp = std::make_shared<PositionComponent>();
    addComponent(positionComp);
}

Entity::~Entity() {}

void Entity::init(const std::string& textureFile, float scale) {
    if (!texture.loadFromFile(textureFile)) {
        throw std::runtime_error("Entity texture not found: " + textureFile);
    }
    sprite.setTexture(texture);
    sprite.setScale(scale, scale);
    bboxSize.x = texture.getSize().x * sprite.getScale().x;
    bboxSize.y = texture.getSize().y * sprite.getScale().y;
    hitboxLocal = { 0.f, 0.f, bboxSize.x, bboxSize.y };
    refreshBoundingBox();
}

void Entity::initSpriteSheet(const std::string& spriteSheetFile) {
    spriteSheet.loadSheet(spriteSheetFile);
    isSpriteSheet = true;
    spriteSheet.setAnimation("Idle", true, true);
    bboxSize.x = spriteSheet.getSpriteSize().x * spriteSheet.getSpriteScale().x;
    bboxSize.y = spriteSheet.getSpriteSize().y * spriteSheet.getSpriteScale().y;
    hitboxLocal = { 0.f, 0.f, bboxSize.x, bboxSize.y };
    refreshBoundingBox();
}

Rectangle Entity::hitboxAt(float x, float y) const {
    const float l = x + hitboxLocal.left;
    const float t = y + hitboxLocal.top;
    return Rectangle(Vector2f(l, t), Vector2f(l + hitboxLocal.width, t + hitboxLocal.height));
}

void Entity::refreshBoundingBox() {
    const sf::Vector2f pos = positionComp->getPosition();
    const Rectangle hb = hitboxAt(pos.x, pos.y);
    boundingBox.setTopLeft(hb.getTopLeft());
    boundingBox.setBottomRight(hb.getBottomRight());
}

void Entity::update(Game* /*game*/, float elapsed) {
    sf::Vector2f pos = positionComp->getPosition();

    if (isSpriteSheet) {
        spriteSheet.getSprite().setPosition(pos.x, pos.y);
        spriteSheet.update(elapsed);
    }
    else {
        sprite.setPosition(pos.x, pos.y);
    }

    refreshBoundingBox();
}

void Entity::draw(Window* window) {
    if (isSpriteSheet)
        window->draw(spriteSheet.getSprite());
    else
        window->draw(sprite);
    if (window->isDebugDraw())
        window->draw(boundingBox.getDrawableRect());
}

void Entity::setPosition(float x, float y) {
    positionComp->setPosition(x, y);
    if (isSpriteSheet)
        spriteSheet.getSprite().setPosition(x, y);
    else
        sprite.setPosition(x, y);
    refreshBoundingBox();
}

sf::Vector2f Entity::getPosition() const {
    return positionComp->getPosition();
}

sf::Vector2f Entity::getCenter() const {
    const sf::Vector2f pos = positionComp->getPosition();
    return { pos.x + hitboxLocal.left + hitboxLocal.width * 0.5f,
             pos.y + hitboxLocal.top + hitboxLocal.height * 0.5f };
}

sf::Vector2i Entity::getTextureSize() const {
    if (isSpriteSheet)
        return spriteSheet.getSpriteSize();
    return sf::Vector2i(texture.getSize().x, texture.getSize().y);
}

sf::Vector2f Entity::getSpriteScale() const {
    if (isSpriteSheet)
        return spriteSheet.getSpriteScale();
    return sprite.getScale();
}
