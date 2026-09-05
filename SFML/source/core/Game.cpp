#include "../../include/core/Game.h"
#include "../../include/entities/Fire.h"
#include "../../include/entities/StaticEntities.h"
#include <iostream>
#include "../../include/core/Command.h"
#include "../../include/core/InputHandler.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include "../../include/systems/Systems.h"
#include "../../include/utils/Observer.h"
#include "../../include/core/AudioManager.h"
#include "../../include/core/ServiceLocator.h"

void Game::registerCollisionCallback(EntityType type, std::function<void(Entity*)> callback) {
    collisionCallbacks[type] = callback;
}

std::shared_ptr<AudioManager> ServiceLocator::audioService = nullptr;

Game::Game(ECSType type)
    : paused(false), entityCounter(1), ecsType(type)
{
    inputHandler = std::make_unique<InputHandler>();

    systems.push_back(std::make_shared<InputSystem>());
    systems.push_back(std::make_shared<MovementSystem>());
    systems.push_back(std::make_shared<ColliderSystem>());
    systems.push_back(std::make_shared<PrintDebugSystem>());
    systems.push_back(std::make_shared<TTLSystem>());

    graphicsSystems.push_back(std::make_shared<GraphicsSystem>());

    // Initialize archetypes if ECS type is ARCHETYPES
    if (ecsType == ECSType::ARCHETYPES) {
        Archetype movableEntities;
        movableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::VELOCITY));
        movableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::POSITION));

        Archetype drawableEntities;
        drawableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::GRAPHICS));

        archetypes.push_back(movableEntities);
        archetypes.push_back(drawableEntities);
    }
}

Game::~Game() {}

void Game::init(std::vector<std::string> lines)
{
    size_t h = lines.size();
    if (h == 0) throw std::runtime_error("No data in level file");
    size_t w = static_cast<size_t>(-1);

    window.loadFont("font/AmaticSC-Regular.ttf");
    window.setTitle("Mini-Game");

    // INIT AUDIO MANAGER and REGISTER SERVICE LOCATOR
    auto audio = std::make_shared<AudioManager>();
    audio->loadSound("pickup", "audio/potion_collect.wav");
    audio->loadSound("fire", "audio/fire.wav");
    audio->loadSound("axe", "audio/sword-slash.wav");
    ServiceLocator::provide(audio);

    auto it = lines.cbegin();
    int row = 0;
    while (it != lines.cend())
    {
        int col = 0;
        if (w == static_cast<size_t>(-1))
        {
            w = it->size();
            buildBoard(w, h);
            initWindow(w, h);
        }
        std::string::const_iterator is = it->cbegin();
        while (is != it->cend())
        {
            switch (*is)
            {
                case '.':
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                    break;
                case 'w':
                    board->addTile(col, row, tileScale, TileType::WALL, "img/wall.png");
                    break;
                case 'x':
                {
                    auto ent = buildEntityAt<Log>("img/log.png", col, row);
                    addEntity(ent);
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                    break;
                }
                case 'p':
                {
                    auto ent = buildEntityAt<Potion>("img/potion.png", col, row);
                    addEntity(ent);
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                    break;
                }
                case '*':
                {
                    player = std::make_shared<Player>();
                    player->initSpriteSheet("img/DwarfSpriteSheet_data.txt");
                    player->positionSprite(row, col, spriteWH, tileScale);
                    addEntity(player);
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");

                    // Observer Pattern: Create and assign
                    achievementObserver = std::make_shared<AchievementObserver>();
                    player->setObserver(achievementObserver);
                    // Register collision callbacks
                    registerCollisionCallback(EntityType::POTION, std::bind(&Player::handlePotionCollision, player.get(), std::placeholders::_1));
                    registerCollisionCallback(EntityType::LOG, std::bind(&Player::handleLogCollision, player.get(), std::placeholders::_1));
                    break;
                }
            }
            col++;
            is++;
        }
        row++;
        it++;
    }

    if (!player) throw std::runtime_error("Level has no player start ('*')");
    flushPendingEntities();
}

void Game::addEntity(std::shared_ptr<Entity> newEntity)
{
    entityCounter++;
    newEntity->setID(entityCounter);
    pendingEntities.push_back(std::move(newEntity));
}

void Game::flushPendingEntities()
{
    for (auto& newEntity : pendingEntities) {
        entities.push_back(newEntity);

        if (ecsType == ECSType::ARCHETYPES) {
            for (auto& archetype : archetypes) {
                if (newEntity->hasComponent(archetype.componentMask)) {
                    archetype.entities.push_back(newEntity);
                }
            }
        }
        else if (ecsType == ECSType::PACKED_ARRAY) {
            packedEntities.insert(newEntity->getID(), newEntity);
        }
    }
    pendingEntities.clear();
}

void Game::handleInput()
{
    window.pollEvents();

    if (window.wasKeyPressed(sf::Keyboard::F1)) {
        window.setDebugDraw(!window.isDebugDraw());
    }

    auto cmd = inputHandler->handleInput(window);
    if (cmd) { cmd->execute(*this); }
}

void Game::update(float elapsed)
{
    if (paused) return;

    runSystems(elapsed);

    for (auto& ent : entities) {
        ent->update(this, elapsed);
    }

    // Collision handling for static entities.
    Rectangle& playerBB = player->getBoundingBox();
    for (auto& ent : entities) {
        if (ent == player || ent->isDeleted()) continue;
        Rectangle& eBB = ent->getBoundingBox();
        if (playerBB.intersects(eBB)) {
            auto it = collisionCallbacks.find(ent->getEntityType());
            if (it != collisionCallbacks.end()) {
                it->second(ent.get());
            }
        }
    }

    removeDeletedEntities();
    flushPendingEntities();
}

void Game::runSystems(float elapsed)
{
    switch (ecsType) {
    case ECSType::ARCHETYPES:   updateArchetypes(elapsed);  break;
    case ECSType::PACKED_ARRAY: updatePackedArray(elapsed); break;
    case ECSType::BIG_ARRAY:
    default:                    bigArray(elapsed);          break;
    }
}

// Big array function
void Game::bigArray(float elapsed) {
    for (const auto& sys : systems) {
        for (auto& ent : entities) {
            if (sys->validate(ent.get())) {
                sys->update(this, ent.get(), elapsed);
            }
        }
    }
}

// New updateArchetypes method (implementing Archetypes ECS logic)
void Game::updateArchetypes(float elapsed) {
    for (auto& archetype : archetypes) {
        for (auto& entity : archetype.entities) {
            for (auto& sys : systems) {
                if (sys->validate(entity.get()))
                    sys->update(this, entity.get(), elapsed);
            }
        }
    }
}

// Packed ECS loop
void Game::updatePackedArray(float elapsed) {
    for (auto& sys : systems) {
        for (auto& ent : packedEntities.getDense()) {
            if (sys->validate(ent.get())) {
                sys->update(this, ent.get(), elapsed);
            }
        }
    }
}

void Game::removeDeletedEntities()
{
    auto isDeleted = [](const std::shared_ptr<Entity>& e) { return e->isDeleted(); };

    if (ecsType == ECSType::ARCHETYPES) {
        for (auto& archetype : archetypes) {
            archetype.entities.erase(
                std::remove_if(archetype.entities.begin(), archetype.entities.end(), isDeleted),
                archetype.entities.end());
        }
    }
    else if (ecsType == ECSType::PACKED_ARRAY) {
        for (auto& e : entities) {
            if (e->isDeleted() && packedEntities.contains(e->getID()))
                packedEntities.remove(e->getID());
        }
    }

    entities.erase(std::remove_if(entities.begin(), entities.end(), isDeleted), entities.end());
}

void Game::render(float /*elapsed*/)
{
    window.beginDraw();
    if (board) { board->draw(&window); }
    for (auto& ent : entities) {
        ent->draw(&window);
    }
    window.drawGUI(*this);
    window.endDraw();
}

sf::Time Game::getElapsed() const
{
    return gameClock.getElapsedTime();
}

void Game::setFPS(int fps)
{
    window.setFPS(fps);
}

void Game::buildBoard(size_t width, size_t height)
{
    board = std::make_unique<Board>(width, height);
}

void Game::initWindow(size_t width, size_t height)
{
    unsigned wdt = static_cast<unsigned>(width * spriteWH * tileScale);
    unsigned hgt = static_cast<unsigned>(height * spriteWH * tileScale);
    window.setup("Mini-Game", sf::Vector2u(wdt, hgt));
}

EntityID Game::getIDCounter()
{
    return entityCounter;
}
