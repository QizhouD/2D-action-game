#include "../../include/entities/Player.h"
#include "../../include/graphics/AnimBase.h"
#include "../../include/entities/Fire.h"
#include "../../include/core/Game.h"
#include <iostream>
#include "../../include/core/Command.h"
#include "../../include/components/InputComponent.h"
#include "../../include/core/ServiceLocator.h"
#include "../../include/entities/StaticEntities.h"

const float Player::playerSpeed = 150.f;
const int   Player::shootingCost = 1;
const float Player::shootCooldownTime = 0.5f;

Player::Player()
    : Entity(EntityType::PLAYER),
    attacking(false),
    shouting(false),
    fireSpawnedThisShout(false),
    wood(0),
    shootCooldown(0)
{
    // Direction comes from input; the component's speed factor scales it to px/s.
    velocity = std::make_shared<VelocityComponent>(playerSpeed);
    addComponent(velocity);

    input = std::make_shared<PlayerInputComponent>();
    addComponent(input);

    healthComp = std::make_shared<HealthComponent>(startingHealth, maxHealth);
    addComponent(healthComp);
}

Player::~Player() {}

void Player::init(const std::string& textureFile, float scale) {
    Entity::init(textureFile, scale);
}

void Player::initSpriteSheet(const std::string& spriteSheetFile) {
    Entity::initSpriteSheet(spriteSheetFile);
}

void Player::setObserver(std::shared_ptr<Observer> obs) {
    observer = obs;
}

std::shared_ptr<Observer> Player::getObserver() const {
    return observer;
}

void Player::startAttack() {
    if (isBusy()) return;
    attacking = true;
    spriteSheet.setAnimation("Attack", true, false, /*restart=*/true);
    if (auto audio = ServiceLocator::getAudio()) audio->playSound("axe");
}

void Player::startShout() {
    if (isBusy()) return;
    if (wood < shootingCost || shootCooldown > 0.f) return;
    shouting = true;
    fireSpawnedThisShout = false;
    spriteSheet.setAnimation("Shout", true, false, /*restart=*/true);
}

void Player::updateMovementAnimation() {
    const sf::Vector2f vel = velocity->getVelocity();
    if (vel.x > 0.f) {
        spriteSheet.setAnimation("Walk", true, true);
        spriteSheet.setSpriteDirection(Direction::Right);
    }
    else if (vel.x < 0.f) {
        spriteSheet.setAnimation("Walk", true, true);
        spriteSheet.setSpriteDirection(Direction::Left);
    }
    else if (vel.y != 0.f) {
        spriteSheet.setAnimation("Walk", true, true);
    }
    else {
        spriteSheet.setAnimation("Idle", true, true);
    }
}

void Player::update(Game* game, float elapsed) {
    if (shootCooldown > 0.f) {
        shootCooldown -= elapsed;
    }

    AnimBase* anim = spriteSheet.getCurrentAnim();

    // A one-shot action ends when its animation stops playing.
    if (isBusy() && anim && !anim->isPlaying()) {
        attacking = false;
        shouting = false;
    }

    // Spawn the fireball once, on the shout's action frames.
    if (shouting && !fireSpawnedThisShout && anim && anim->isInAction()) {
        fireSpawnedThisShout = true;
        game->addEntity(createFire());
        if (auto audio = ServiceLocator::getAudio()) audio->playSound("fire");
        addWood(-shootingCost);
        shootCooldown = shootCooldownTime;
        if (observer) observer->onShoutPerformed();
    }

    if (!isBusy()) {
        updateMovementAnimation();
    }

    // Position/velocity integration is done by MovementSystem; this only
    // syncs sprite + bounding box and advances the animation.
    Entity::update(game, elapsed);
}

void Player::draw(Window* window) {
    Entity::draw(window);
}

void Player::addWood(int w) {
    wood += w;
    if (wood > maxWood) { wood = maxWood; }
    if (wood < 0) { wood = 0; }
}

std::shared_ptr<Fire> Player::createFire() const {
    auto fireEntity = std::make_shared<Fire>();
    fireEntity->init("img/Fire.png", 1.f);

    // Centre the fireball on the (scaled) player sprite.
    const sf::Vector2f pos = getPosition();
    const sf::Vector2i tex = getTextureSize();
    const sf::Vector2f scl = getSpriteScale();
    const sf::Vector2i fireTex = fireEntity->getTextureSize();
    const float cx = pos.x + tex.x * scl.x * 0.5f - fireTex.x * 0.5f;
    const float cy = pos.y + tex.y * scl.y * 0.5f - fireTex.y * 0.5f;
    fireEntity->setPosition(cx, cy);

    if (auto fireVel = fireEntity->getVelocityComp()) {
        const float dir = (spriteSheet.getSpriteDirection() == Direction::Left) ? -1.f : 1.f;
        fireVel->setVelocity(dir, 0.f);
    }
    return fireEntity;
}

void Player::positionSprite(int row, int col, int spriteWH, float tileScale) {
    sf::Vector2f scaleV2f = getSpriteScale();
    sf::Vector2i texSize = getTextureSize();
    float x = col * spriteWH * tileScale;
    float y = row * spriteWH * tileScale;
    float spriteSizeY = scaleV2f.y * texSize.y;
    float cntrFactorY = (spriteWH * tileScale - spriteSizeY);
    float cntrFactorX = cntrFactorY * 0.5f;
    setPosition(x + cntrFactorX, y + cntrFactorY);
    if (velocity) {
        velocity->setVelocity(0.f, 0.f);
    }
}

void Player::handlePotionCollision(Entity* potion) { 
    auto pot = dynamic_cast<Potion*>(potion);
    if (pot && healthComp) {
        int potionHealth = pot->getHealth();
        healthComp->changeHealth(potionHealth);
        std::cout << "Potion restores: " << potionHealth
            << ", Player Health: " << healthComp->getHealth() << std::endl;
        if (observer) observer->onPotionCollected();
        if (auto audio = ServiceLocator::getAudio()) audio->playSound("pickup");
        potion->deleteEntity();
    }
}

void Player::handleLogCollision(Entity* log) {
    if (!isAttacking() || !spriteSheet.getCurrentAnim() || !spriteSheet.getCurrentAnim()->isInAction())
        return;

    auto logObj = dynamic_cast<Log*>(log);
    if (logObj) {
        int logWood = logObj->getWood();
        addWood(logWood);
        std::cout << "Wood collected: " << logWood
            << ", Total Wood: " << getWood() << std::endl;
        log->deleteEntity();
    }
}
