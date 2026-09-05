#include "../../include/entities/Player.h"
#include "../../include/graphics/AnimBase.h"
#include "../../include/entities/Fire.h"
#include "../../include/entities/Enemy.h"
#include "../../include/core/Game.h"
#include <cmath>
#include <iostream>
#include "../../include/core/Command.h"
#include "../../include/components/InputComponent.h"
#include "../../include/core/ServiceLocator.h"
#include "../../include/entities/StaticEntities.h"

const float Player::playerSpeed = 150.f;
const int   Player::shootingCost = 1;
const float Player::shootCooldownTime = 0.5f;
const int   Player::axeDamage = 15;
const float Player::axeReach = 48.f;
const float Player::invulnerableTime = 1.0f;

Player::Player()
    : Entity(EntityType::PLAYER),
    attacking(false),
    shouting(false),
    fireSpawnedThisShout(false),
    dead(false),
    invulnTimer(0.f),
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
    // The dwarf occupies roughly the middle of its 32x32 cell; use a hitbox
    // that matches the visible body so 1-tile corridors are comfortable.
    const sf::Vector2f size = getSpriteSize();      // 96x96 at scale 3
    const float w = size.x * 0.46f;
    const float h = size.y * 0.62f;
    setHitbox((size.x - w) * 0.5f, size.y - h - size.y * 0.06f, w, h);
}

void Player::setObserver(std::shared_ptr<Observer> obs) {
    observer = obs;
}

std::shared_ptr<Observer> Player::getObserver() const {
    return observer;
}

void Player::resetStats() {
    healthComp->changeHealth(startingHealth - healthComp->getHealth());
    wood = 0;
    onLevelStart();
}

void Player::onLevelStart() {
    attacking = shouting = fireSpawnedThisShout = false;
    dead = false;
    invulnTimer = 0.f;
    shootCooldown = 0.f;
    hitThisSwing.clear();
    velocity->setVelocity(0.f, 0.f);
    spriteSheet.getSprite().setColor(sf::Color::White);
    spriteSheet.setAnimation("Idle", true, true, /*restart=*/true);
}

void Player::startAttack() {
    if (isBusy() || dead) return;
    attacking = true;
    hitThisSwing.clear();
    spriteSheet.setAnimation("Attack", true, false, /*restart=*/true);
    if (auto audio = ServiceLocator::getAudio()) audio->playSound("axe");
}

void Player::startShout() {
    if (isBusy() || dead) return;
    if (wood < shootingCost || shootCooldown > 0.f) return;
    shouting = true;
    fireSpawnedThisShout = false;
    spriteSheet.setAnimation("Shout", true, false, /*restart=*/true);
}

bool Player::takeDamage(int amount) {
    if (dead || invulnTimer > 0.f || amount <= 0) return false;
    healthComp->changeHealth(-amount);
    invulnTimer = invulnerableTime;
    if (healthComp->getHealth() <= 0) {
        dead = true;
        attacking = shouting = false;
        velocity->setVelocity(0.f, 0.f);
        spriteSheet.getSprite().setColor(sf::Color::White);
        spriteSheet.setAnimation("Death", true, false, /*restart=*/true);
    }
    return true;
}

bool Player::isDeathAnimationDone() const {
    if (!dead) return false;
    const AnimBase* anim = spriteSheet.getCurrentAnim();
    return anim == nullptr || !anim->isPlaying();
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
    if (dead) {
        velocity->setVelocity(0.f, 0.f);
        Entity::update(game, elapsed);
        return;
    }

    if (shootCooldown > 0.f) shootCooldown -= elapsed;
    if (invulnTimer > 0.f) invulnTimer -= elapsed;

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

    // Blink while invulnerable.
    if (invulnTimer > 0.f) {
        const bool visible = static_cast<int>(invulnTimer * 12.f) % 2 == 0;
        spriteSheet.getSprite().setColor(visible ? sf::Color::White : sf::Color(255, 255, 255, 90));
    }
    else {
        spriteSheet.getSprite().setColor(sf::Color::White);
    }

    // Position/velocity integration is done by MovementSystem; this only
    // syncs sprite + bounding box and advances the animation.
    Entity::update(game, elapsed);
}

void Player::draw(Window* window) {
    Entity::draw(window);
    if (window->isDebugDraw() && isSwingActive()) {
        const Rectangle atk = getAttackBox();
        const auto& tl = atk.getTopLeft();
        const auto& br = atk.getBottomRight();
        sf::RectangleShape r({ br.x - tl.x, br.y - tl.y });
        r.setPosition(tl.x, tl.y);
        r.setFillColor(sf::Color(255, 0, 0, 60));
        r.setOutlineColor(sf::Color::Red);
        r.setOutlineThickness(2.f);
        window->draw(r);
    }
}

bool Player::isSwingActive() const {
    const AnimBase* anim = spriteSheet.getCurrentAnim();
    return attacking && !dead && anim && anim->isInAction();
}

Rectangle Player::getAttackBox() const {
    // Body hitbox extended forward by the axe reach, with a little vertical slack.
    const sf::Vector2f pos = getPosition();
    float left   = pos.x + hitboxLocal.left;
    float right  = left + hitboxLocal.width;
    const float top    = pos.y + hitboxLocal.top - 10.f;
    const float bottom = pos.y + hitboxLocal.top + hitboxLocal.height + 10.f;
    if (spriteSheet.getSpriteDirection() == Direction::Left) left -= axeReach;
    else                                                    right += axeReach;
    return Rectangle(Vector2f(left, top), Vector2f(right, bottom));
}

void Player::chopLog(Entity* log) {
    if (dead || log->isDeleted()) return;
    auto logObj = dynamic_cast<Log*>(log);
    if (!logObj) return;
    addWood(logObj->getWood());
    if (observer) observer->onWoodCollected(logObj->getWood());
    log->deleteEntity();
}

void Player::hitEnemy(Entity* entity) {
    if (dead) return;
    auto enemy = dynamic_cast<Enemy*>(entity);
    if (!enemy || !enemy->isAlive()) return;
    if (hitThisSwing.insert(enemy->getID()).second) {
        enemy->takeDamage(axeDamage);   // kills are credited by Game
    }
}

void Player::addWood(int w) {
    wood += w;
    if (wood > maxWood) { wood = maxWood; }
    if (wood < 0) { wood = 0; }
}

std::shared_ptr<Fire> Player::createFire() const {
    auto fireEntity = std::make_shared<Fire>();
    fireEntity->init("img/Fire.png", 1.f);

    // Centre the fireball on the player's hitbox and push it out of the body
    // so it does not immediately clip the wall the player is leaning on.
    const sf::Vector2f c = getCenter();
    const sf::Vector2f fireSize = fireEntity->getSpriteSize();
    const float dir = (spriteSheet.getSpriteDirection() == Direction::Left) ? -1.f : 1.f;
    fireEntity->setPosition(c.x - fireSize.x * 0.5f + dir * hitboxLocal.width * 0.5f,
                            c.y - fireSize.y * 0.5f);

    if (auto fireVel = fireEntity->getVelocityComp()) {
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
    if (dead) return;
    auto pot = dynamic_cast<Potion*>(potion);
    if (pot && healthComp) {
        healthComp->changeHealth(pot->getHealth());
        if (observer) observer->onPotionCollected();
        if (auto audio = ServiceLocator::getAudio()) audio->playSound("pickup");
        potion->deleteEntity();
    }
}

void Player::handleEnemyCollision(Entity* entity) {
    if (dead) return;
    auto enemy = dynamic_cast<Enemy*>(entity);
    if (!enemy || !enemy->isAlive()) return;
    takeDamage(enemy->getContactDamage());
}
