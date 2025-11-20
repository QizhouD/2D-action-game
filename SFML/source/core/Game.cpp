#include "../../include/core/Game.h"
#include "../../include/entities/Fire.h"
#include "../../include/entities/StaticEntities.h"
#include <iostream>
#include "../../include/core/Command.h"
#include "../../include/core/InputHandler.h"
#include <algorithm>
#include <sstream>
#include "../../include/systems/Systems.h"
#include "../../include/utils/Observer.h"
#include "../../include/core/AudioManager.h"
#include "../../include/core/ServiceLocator.h"
#include "../../include/entities/Mushroom.h"
#include <SFML/Window/Keyboard.hpp>
#include <fstream>
#include <cmath>

void Game::registerCollisionCallback(EntityType type, std::function<void(Entity*)> callback) {
    collisionCallbacks[type] = callback;
}

std::shared_ptr<AudioManager> ServiceLocator::audioService = nullptr;

Game::Game(ECSType type)
    : paused(false), entityCounter(1), ecsType(type)
{
    inputHandler = std::make_unique<InputHandler>();

    // RNG seed
    std::random_device rd; rng.seed(rd());

    //added the remaining systems
    systems.push_back(std::make_shared<InputSystem>());
    systems.push_back(std::make_shared<MovementSystem>());
    systems.push_back(std::make_shared<ColliderSystem>());
    systems.push_back(std::make_shared<GameplaySystem>());
    systems.push_back(std::make_shared<PrintDebugSystem>());

    graphicsSystems.push_back(std::make_shared<GraphicsSystem>());

    // Initialize archetypes if ECS type is ARCHETYPES
    if (ecsType == ECSType::ARCHETYPES) {
        // Archetype initialization
        Archetype movableEntities;
        movableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::VELOCITY));
        movableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::POSITION));

        Archetype drawableEntities;
        drawableEntities.componentMask.turnOnBit(static_cast<unsigned int>(ComponentID::GRAPHICS));

        archetypes.push_back(movableEntities);
        archetypes.push_back(drawableEntities);
    }


    //Initialized TTLSystem and add it to systems vector
    auto ttlSystem = std::make_shared<TTLSystem>();
    systems.push_back(ttlSystem);

}

// Big array function
void Game::bigArray(float elapsed) {
    for (const auto& sys : systems) {
        for (auto& ent : entities) {
            if (!ent) { continue; }
            if (sys->validate(ent.get())) {
                sys->update(this, ent.get(), elapsed);
            }
        }
    }
}



Game::~Game() {}

void Game::parseLevelLines(const std::vector<std::string>& lines)
{
    size_t h = lines.size();
    if (h == 0) throw std::exception("No data in level file");
    size_t w = static_cast<size_t>(-1);

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
                case 'm':
                {
                    auto mush = buildEntityAt<Mushroom>("img/mushroom50-50.png", col, row);
                    addEntity(mush);
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                    break;
                }
                case 'g':
                {
                    auto goal = buildEntityAt<Goal>("img/potion.png", col, row);
                    addEntity(goal);
                    board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                    break;
                }
            }
            col++;
            is++;
        }
        row++;
        it++;
    }
}

void Game::init(std::vector<std::string> lines)
{
    window.loadFont("font/AmaticSC-Regular.ttf");
    window.setTitle("Mini-Game");

    // INIT AUDIO MANAGER and REGISTER SERVICE LOCATOR
    auto audio = std::make_shared<AudioManager>();
    audio->loadSound("pickup", "audio/potion_collect.wav");
    audio->loadSound("fire", "audio/fire.wav");
    audio->loadSound("axe", "audio/sword-slash.wav");
    ServiceLocator::provide(audio);

    currentLevelIndex = 0;
    entities.clear();
    board.reset();
    player.reset();

    parseLevelLines(lines);
}

void Game::addEntity(std::shared_ptr<Entity> newEntity)
{
    if (!newEntity) { return; }
    entityCounter++;
    newEntity->setID(entityCounter);
    entities.push_back(newEntity);

    // Add entity to corresponding archetypes if using Archetypes ECS
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

void Game::handleInput()
{
    // If we are in the main menu, pressing Enter will start the game
    if (isInMenu()) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
            startGame();
        }
        window.update();
        return;
    }

    auto cmd = inputHandler->handleInput();
    if (cmd) { cmd->execute(*this); }
    if (player) { player->handleInput(*this); }
}

void Game::update(float elapsed)
{
    // In menu, only process window events and skip game updates
    if (isInMenu()) {
        window.update();
        return;
    }

    if (!paused) {
        bigArray(elapsed);

        for (auto& ent : entities) {
            if (!ent) { continue; }
            ent->update(this, elapsed);
        }
    }

    if (ecsType == ECSType::ARCHETYPES)
        updateArchetypes(elapsed);  // Use Archetypes ECS
    else if (ecsType == ECSType::PACKED_ARRAY)
        updatePackedArray(elapsed); // Packed ECS update
    else
        bigArray(elapsed);          // Use Big Array ECS (existing)

    // Resolve tile collisions for the player (terrain vs player)
    resolveTileCollisionsForPlayer(elapsed);

    // Collision handling for static entities.
    if (player) {
        Rectangle& playerBB = player->getBoundingBox();
        for (auto& ent : entities) {
            if (!ent || ent == player) continue;
            Rectangle& eBB = ent->getBoundingBox();
            if (playerBB.intersects(eBB)) {
                auto it = collisionCallbacks.find(ent->getEntityType());
                if (it != collisionCallbacks.end()) {                    
                    it->second(ent.get());                               
                }                                                       
                
            }
        }
    }

    // Remove deleted entities.
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [](std::shared_ptr<Entity>& e) { return !e || e->isDeleted(); }),
        entities.end()
    );

    // Random Mushroom spawning when playing
    if (!isInMenu() && !paused) {
        // Count existing mushrooms
        int mushCount = 0;
        for (const auto& e : entities) {
            if (e && e->getEntityType() == EntityType::MUSHROOM) mushCount++;
        }
        spawnTimer += elapsed;
        if (mushCount < maxMushrooms && spawnTimer >= nextSpawnInterval) {
            // choose random tile within window bounds, but avoid spawning inside solid terrain
            auto ws = window.getWindowSize();
            int cols = static_cast<int>(ws.x / (spriteWH * tileScale));
            int rows = static_cast<int>(ws.y / (spriteWH * tileScale));
            if (cols > 0 && rows > 0) {
                std::uniform_int_distribution<int> cx(0, cols - 1);
                std::uniform_int_distribution<int> cy(0, rows - 1);

                // Try a few times to find a non-solid tile that's far enough from the player
                const int maxAttempts = 20;
                for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                    int col = cx(rng);
                    int row = cy(rng);

                    // Skip if out of board bounds or solid
                    if (!board || !board->inBounds(col, row) || board->isSolid(col, row)) {
                        continue;
                    }

                    // avoid spawning too close to player
                    bool farEnough = true;
                    if (player) {
                        float px = player->getPosition().x;
                        float py = player->getPosition().y;
                        float x = col * spriteWH * tileScale;
                        float y = row * spriteWH * tileScale;
                        float dx = px - x; float dy = py - y;
                        farEnough = (dx*dx + dy*dy) > (200.f * 200.f);
                    }
                    if (!farEnough) {
                        continue;
                    }

                    auto mush = buildEntityAt<Mushroom>("img/mushroom50-50.png", col, row);
                    addEntity(mush);
                    break; // spawned successfully
                }
            }
            spawnTimer = 0.f;
            std::uniform_real_distribution<float> nd(spawnIntervalMin, spawnIntervalMax);
            nextSpawnInterval = nd(rng);
        }
    }

    window.update();
}

// New updateArchetypes method (implementing Archetypes ECS logic)
void Game::updateArchetypes(float elapsed) {
    // Iterate over each archetype
    for (auto& archetype : archetypes) {
        for (auto& entity : archetype.entities) {
            if (!entity) { continue; }
            // Iterate through systems and update relevant entities
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
            if (!ent) { continue; }
            if (sys->validate(ent.get())) {
                sys->update(this, ent.get(), elapsed);
            }
        }
    }
}

void Game::resolveTileCollisionsForPlayer(float elapsed)
{
    if (!player || !board) return;

    auto velComp = player->getVelocityComp();
    sf::Vector2f vel(0.f, 0.f);
    if (velComp) vel = velComp->getVelocity();

    // Player current world position (top-left) and size from bounding box
    auto& bb = player->getBoundingBox();
    Vector2f tl = bb.getTopLeft();
    Vector2f br = bb.getBottomRight();
    sf::Vector2f size(br.x - tl.x, br.y - tl.y);
    sf::Vector2f pos = player->getPosition();

    const float tileSize = spriteWH * tileScale;

    // ---- Resolve X axis ----
    float nextX = pos.x + vel.x * elapsed;
    sf::FloatRect aabbX(nextX, pos.y, size.x, size.y);

    int topRow = board->worldToGrid(aabbX.top, tileSize);
    int bottomRow = board->worldToGrid(aabbX.top + aabbX.height - 1.f, tileSize);
    if (vel.x > 0.f) {
        int rightCol = board->worldToGrid(aabbX.left + aabbX.width - 1.f, tileSize);
        for (int gy = topRow; gy <= bottomRow; ++gy) {
            if (!board->isSolid(rightCol, gy)) continue;
            sf::FloatRect tb = board->tileBounds(rightCol, gy);
            if (aabbX.intersects(tb)) {
                nextX = tb.left - size.x;
                aabbX.left = nextX;
                if (velComp) vel.x = 0.f;
                break; // resolved against the nearest tile on this side
            }
        }
    }
    else if (vel.x < 0.f) {
        int leftCol = board->worldToGrid(aabbX.left, tileSize);
        for (int gy = topRow; gy <= bottomRow; ++gy) {
            if (!board->isSolid(leftCol, gy)) continue;
            sf::FloatRect tb = board->tileBounds(leftCol, gy);
            if (aabbX.intersects(tb)) {
                nextX = tb.left + tb.width;
                aabbX.left = nextX;
                if (velComp) vel.x = 0.f;
                break;
            }
        }
    }
    // Apply X correction
    player->setPosition(nextX, pos.y);
    pos.x = nextX;

    // ---- Resolve Y axis ----
    float nextY = pos.y + vel.y * elapsed;
    sf::FloatRect aabbY(pos.x, nextY, size.x, size.y);

    int leftCol = board->worldToGrid(aabbY.left, tileSize);
    int rightCol = board->worldToGrid(aabbY.left + aabbY.width - 1.f, tileSize);

    if (vel.y > 0.f) {
        int bottomRow2 = board->worldToGrid(aabbY.top + aabbY.height - 1.f, tileSize);
        for (int gx = leftCol; gx <= rightCol; ++gx) {
            if (!board->isSolid(gx, bottomRow2)) continue;
            sf::FloatRect tb = board->tileBounds(gx, bottomRow2);
            if (aabbY.intersects(tb)) {
                nextY = tb.top - size.y;
                aabbY.top = nextY;
                if (velComp) vel.y = 0.f;
                break;
            }
        }
    }
    else if (vel.y < 0.f) {
        int topRow2 = board->worldToGrid(aabbY.top, tileSize);
        for (int gx = leftCol; gx <= rightCol; ++gx) {
            if (!board->isSolid(gx, topRow2)) continue;
            sf::FloatRect tb = board->tileBounds(gx, topRow2);
            if (aabbY.intersects(tb)) {
                nextY = tb.top + tb.height;
                aabbY.top = nextY;
                if (velComp) vel.y = 0.f;
                break;
            }
        }
    }

    // Apply Y correction
    player->setPosition(pos.x, nextY);

    // Update possibly zeroed velocity back to component
    if (velComp) velComp->setVelocity(vel.x, vel.y);
}

void Game::render(float elapsed)
{
    window.beginDraw();
    if (board) { board->draw(&window); }
    for (auto& ent : entities) {
        ent->draw(&window);
    }
    window.drawGUI(*this);
    window.endDraw();
}

bool Game::loadLevelByIndex(int index)
{
    std::ostringstream ss; ss << "levels/lvl" << index << ".txt";
    std::ifstream levelRead{ ss.str() };
    if (!levelRead) {
        std::cout << "[Game] Level file not found: " << ss.str() << std::endl;
        return false;
    }
    std::vector<std::string> lines;
    while (levelRead) {
        std::string line;
        std::getline(levelRead, line);
        if (!levelRead && line.empty()) break;
        lines.emplace_back(line);
    }

    // reset world state (keep window/audio)
    entities.clear();
    board.reset();
    player.reset();
    achievementObserver.reset();

    parseLevelLines(lines);

    currentLevelIndex = index;
    // reset spawn timer interval
    spawnTimer = 0.f;
    std::uniform_real_distribution<float> nd(spawnIntervalMin, spawnIntervalMax);
    nextSpawnInterval = nd(rng);

    // ensure we are in playing state
    gameState = GameState::Playing;
    return true;
}

bool Game::loadNextLevel()
{
    int nextIndex = currentLevelIndex + 1;
    return loadLevelByIndex(nextIndex);
}

sf::Time Game::getElapsed() const
{
    return gameClock.getElapsedTime();
}

void Game::setFPS(int fps)
{
    std::cout << "FPS: " << fps << std::endl;
}

void Game::buildBoard(size_t width, size_t height)
{
    board = std::make_unique<Board>(width, height);
}

void Game::initWindow(size_t width, size_t height)
{
    // Compute desired window size from level grid and tile scaling
    unsigned int desiredW = static_cast<unsigned int>(width * spriteWH * tileScale);
    unsigned int desiredH = static_cast<unsigned int>(height * spriteWH * tileScale);

    // Fit to desktop with proportional scaling (letterbox handled by Window)
    auto dm = sf::VideoMode::getDesktopMode();
    float maxW = dm.width * 0.95f;  // leave some margin for OS decorations/taskbar
    float maxH = dm.height * 0.95f;

    float scale = 1.f;
    if (desiredW > maxW || desiredH > maxH) {
        float sx = maxW / static_cast<float>(desiredW);
        float sy = maxH / static_cast<float>(desiredH);
        scale = std::min(sx, sy);
    }

    unsigned int fittedW = static_cast<unsigned int>(std::floor(desiredW * scale));
    unsigned int fittedH = static_cast<unsigned int>(std::floor(desiredH * scale));
    // Ensure a reasonable minimum size
    fittedW = std::max(640u, fittedW);
    fittedH = std::max(360u, fittedH);

    // Set the logical view size so Window can keep aspect with letterbox
    window.setLogicalSize(sf::Vector2u(desiredW, desiredH));
    window.setSize(sf::Vector2u(fittedW, fittedH));
    window.redraw();
}

EntityID Game::getIDCounter()
{
    return entityCounter;
}

std::shared_ptr<Entity> Game::getEntity(unsigned int idx)
{
    if (idx < 1 || idx > entities.size())
        return nullptr;
    return entities[idx - 1];
}
