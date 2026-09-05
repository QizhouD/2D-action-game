#pragma once
#include "../../include/graphics/Window.h"
#include "../../include/graphics/SpriteSheet.h"
#include "../../include/utils/Rectangle.h"
#include "../../include/components/PositionComponent.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include "../../include/utils/Bitmask.h"

using EntityID = unsigned int;
class Game;
class Component;
class TTLComponent;

enum class EntityType {
    UNDEFINED = -1,
    PLAYER = 0,
    POTION = 1,
    LOG = 2,
    FIRE = 3,
    ENEMY = 4
};

class Entity {
public:
    Entity();
    Entity(EntityType et);
    virtual ~Entity();

    virtual void init(const std::string& textureFile, float scale);
    virtual void initSpriteSheet(const std::string& spriteSheetFile);
    virtual void update(Game* game, float elapsed);
    virtual void draw(Window* window);

    void setID(EntityID entId) { id = entId; }
    EntityID getID() const { return id; }

    // Position wrappers using PositionComponent.
    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;

    sf::Vector2i getTextureSize() const;
    sf::Vector2f getSpriteScale() const;
    // Full sprite size in world pixels (texture * scale).
    sf::Vector2f getSpriteSize() const { return bboxSize; }
    sf::Vector2f getCenter() const;

    // --- Collision -------------------------------------------------------
    // The hitbox is a rectangle relative to the sprite's top-left corner. By
    // default it covers the whole sprite; entities can shrink it (setHitbox).
    void setHitbox(float offX, float offY, float w, float h) { hitboxLocal = { offX, offY, w, h }; }
    const sf::FloatRect& getHitboxLocal() const { return hitboxLocal; }
    Rectangle hitboxAt(float x, float y) const;
    Rectangle& getBoundingBox() { return boundingBox; }

    // Whether MovementSystem should stop this entity at WALL tiles.
    virtual bool blocksOnWalls() const { return true; }
    // Called by MovementSystem when a move was rejected by a wall.
    virtual void onWallHit(Game* /*game*/) {}

    const SpriteSheet* getSpriteSheet() const { return &spriteSheet; }
    EntityType getEntityType() const { return type; }

    bool isDeleted() const { return deleted; }
    void deleteEntity() { deleted = true; }

    // Access to the position component.
    std::shared_ptr<PositionComponent> getPositionComp() const { return positionComp; }

    Bitmask getComponentSet() const { return componentSet; }

    void addComponent(std::shared_ptr<Component> component) {
        ComponentID id = component->getID();
        componentSet.turnOnBit(static_cast<unsigned int>(id));
        components[id] = component;
    }

    std::shared_ptr<Component> getComponent(ComponentID id) const {
        auto found = components.find(id);
        if (found != components.end()) {
            return found->second;
        }
        return nullptr;
    }

    bool hasComponent(Bitmask mask) const {
        return componentSet.contains(mask);
    }

    virtual std::shared_ptr<TTLComponent> getTTLComponent() const { return nullptr; }

protected:
    void refreshBoundingBox();

    EntityType type;
    EntityID id;
    std::shared_ptr<PositionComponent> positionComp;
    Rectangle boundingBox;
    sf::Vector2f bboxSize;
    sf::FloatRect hitboxLocal;
    bool isSpriteSheet;
    SpriteSheet spriteSheet;
    const sf::Texture* texture = nullptr;   // owned by ResourceManager
    sf::Sprite sprite;
    bool deleted;
    Bitmask componentSet;
    std::unordered_map<ComponentID, std::shared_ptr<Component>> components;
};
