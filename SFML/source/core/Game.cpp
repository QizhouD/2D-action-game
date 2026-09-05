#include "../../include/core/Game.h"
#include "../../include/entities/Fire.h"
#include "../../include/entities/Enemy.h"
#include "../../include/entities/StaticEntities.h"
#include <iostream>
#include <fstream>
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
    : state(GameState::MainMenu)
    , currentLevel(0)
    , enemiesAlive(0)
    , lastPlayerHealth(0)
    , levelClearDelay(0.f)
    , entityCounter(1)
    , ecsType(type)
    , rng(std::random_device{}())
{
    inputHandler = std::make_unique<InputHandler>();

    // Order matters: input decides direction, AI decides direction, movement
    // integrates, TTL expires projectiles.
    systems.push_back(std::make_shared<InputSystem>());
    systems.push_back(std::make_shared<AISystem>());
    systems.push_back(std::make_shared<MovementSystem>());
    systems.push_back(std::make_shared<ColliderSystem>());
    systems.push_back(std::make_shared<PrintDebugSystem>());
    systems.push_back(std::make_shared<TTLSystem>());

    graphicsSystems.push_back(std::make_shared<GraphicsSystem>());

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

// ---------------------------------------------------------------------------
// Boot / assets
// ---------------------------------------------------------------------------

void Game::init(const std::string& levelListFile)
{
    window.loadFont("font/AmaticSC-Regular.ttf");
    window.setTitle("Dwarf & Fire");
    hud.init(window.getGUIFont(), "img/log.png");

    auto audio = std::make_shared<AudioManager>();
    audio->loadSound("pickup", "audio/potion_collect.wav");
    audio->loadSound("fire", "audio/fire.wav");
    audio->loadSound("axe", "audio/sword-slash.wav");
    ServiceLocator::provide(audio);

    // The player lives across levels; only its position is reset per level.
    player = std::make_shared<Player>();
    player->initSpriteSheet("img/DwarfSpriteSheet_data.txt");

    achievementObserver = std::make_shared<AchievementObserver>(
        [this](const std::string& msg) { hud.pushToast(msg); });
    player->setObserver(achievementObserver);

    // Body-contact callbacks. Logs are handled by the axe swing, not by touch.
    registerCollisionCallback(EntityType::POTION, [this](Entity* e) { player->handlePotionCollision(e); });
    registerCollisionCallback(EntityType::ENEMY,  [this](Entity* e) { player->handleEnemyCollision(e); });

    loadLevelList(levelListFile);
    loadLevel(0);                 // shown behind the main menu
    setState(GameState::MainMenu);
}

void Game::loadLevelList(const std::string& file)
{
    levelFiles.clear();
    // Level names are relative to the directory of the list file.
    std::string dir;
    const size_t slash = file.find_last_of("/\\");
    if (slash != std::string::npos) dir = file.substr(0, slash + 1);

    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        levelFiles.push_back(dir + line);
    }
    if (levelFiles.empty()) {
        levelFiles.push_back("levels/lvl0.txt");
    }
}

// ---------------------------------------------------------------------------
// Level flow
// ---------------------------------------------------------------------------

void Game::placeInTile(Entity& ent, int col, int row) const
{
    const float tile = spriteWH * tileScale;
    const sf::Vector2f size = ent.getSpriteSize();
    ent.setPosition(col * tile + (tile - size.x) * 0.5f,
                    row * tile + (tile - size.y));   // feet on the tile's bottom edge
}

void Game::loadLevel(int index)
{
    if (index < 0 || index >= static_cast<int>(levelFiles.size()))
        throw std::out_of_range("Level index out of range");
    currentLevel = index;

    std::ifstream in(levelFiles[index]);
    if (!in) throw std::runtime_error("Level file not found: " + levelFiles[index]);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    if (lines.empty()) throw std::runtime_error("No data in level file " + levelFiles[index]);

    // Reset world state (the player object survives).
    entities.clear();
    pendingEntities.clear();
    for (auto& a : archetypes) a.entities.clear();
    packedEntities = PackedArray<Entity>();
    hud.clearToasts();
    enemiesAlive = 0;
    levelClearDelay = 0.f;

    const size_t h = lines.size();
    size_t w = 0;
    for (const auto& l : lines) w = std::max(w, l.size());

    const float tile = spriteWH * tileScale;
    board = std::make_unique<Board>(w, h, tile);

    const sf::Vector2u logical(static_cast<unsigned>(w * tile), static_cast<unsigned>(h * tile));
    if (!window.isCreated()) window.setup("Dwarf & Fire", logical);
    else                     window.setLogicalSize(logical);

    bool playerPlaced = false;
    int potions = 0;

    for (int row = 0; row < static_cast<int>(h); ++row) {
        for (int col = 0; col < static_cast<int>(w); ++col) {
            const char c = col < static_cast<int>(lines[row].size()) ? lines[row][col] : 'w';
            switch (c) {
            case 'w':
                board->addTile(col, row, tileScale, TileType::WALL, "img/wall.png");
                continue;
            case 'o':
                board->addTile(col, row, tileScale, TileType::EXIT, "img/floor.png");
                continue;
            default:
                board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png");
                break;
            }

            switch (c) {
            case 'x':
                addEntity(buildEntityAt<Log>("img/log.png", col, row));
                break;
            case 'p':
                addEntity(buildEntityAt<Potion>("img/potion.png", col, row));
                ++potions;
                break;
            case 'e':
            case 'B': {
                const EnemyStats& st = (c == 'B') ? EnemyStats::bossMushroom() : EnemyStats::mushroom();
                auto enemy = std::make_shared<Enemy>(st);
                enemy->init(st.texture, st.scale);
                placeInTile(*enemy, col, row);
                addEntity(enemy);
                ++enemiesAlive;
                break;
            }
            case '*':
                player->positionSprite(row, col, spriteWH, tileScale);
                playerPlaced = true;
                break;
            default:
                break;
            }
        }
    }

    if (!playerPlaced) throw std::runtime_error("Level has no player start ('*'): " + levelFiles[index]);

    player->onLevelStart();
    addEntity(player);
    flushPendingEntities();

    achievementObserver->startLevel(potions);
    lastPlayerHealth = player->getHealthComp()->getHealth();
    board->setExitActive(enemiesAlive == 0);
}

void Game::startNewGame()
{
    achievementObserver->resetAll();
    player->resetStats();
    loadLevel(0);
    setState(GameState::Playing);
    hud.pushToast("Level 1: clear the mushrooms!");
}

void Game::restartLevel()
{
    player->resetStats();
    loadLevel(currentLevel);
    setState(GameState::Playing);
}

void Game::nextLevel()
{
    if (currentLevel + 1 >= static_cast<int>(levelFiles.size())) {
        setState(GameState::Victory);
        return;
    }
    loadLevel(currentLevel + 1);
    setState(GameState::Playing);
    hud.pushToast("Level " + std::to_string(currentLevel + 1));
}

void Game::setState(GameState s)
{
    state = s;
}

void Game::togglePause()
{
    if (state == GameState::Playing)      setState(GameState::Paused);
    else if (state == GameState::Paused)  setState(GameState::Playing);
}

// ---------------------------------------------------------------------------
// Entities
// ---------------------------------------------------------------------------

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

void Game::spawnPotionAt(const sf::Vector2f& center)
{
    auto potion = std::make_shared<Potion>();
    potion->init("img/potion.png", itemScale);
    const sf::Vector2f size = potion->getSpriteSize();
    potion->setPosition(center.x - size.x * 0.5f, center.y - size.y * 0.5f);
    addEntity(potion);
}

void Game::onEnemyKilled(Enemy* enemy)
{
    achievementObserver->onEnemyKilled(enemy->getStats().score);
    std::uniform_real_distribution<float> roll(0.f, 1.f);
    if (roll(rng) < enemy->getStats().potionDropChance) {
        spawnPotionAt(enemy->getCenter());
    }
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Game::handleInput()
{
    window.pollEvents();

    if (window.wasKeyPressed(sf::Keyboard::F1)) {
        window.setDebugDraw(!window.isDebugDraw());
    }

    switch (state) {
    case GameState::MainMenu:
        if (window.wasKeyPressed(sf::Keyboard::Enter)) startNewGame();
        else if (window.wasKeyPressed(sf::Keyboard::Escape)) window.close();
        break;

    case GameState::Playing:
    case GameState::Paused:
        if (auto cmd = inputHandler->handleInput(window)) cmd->execute(*this);
        if (state == GameState::Paused) {
            if (window.wasKeyPressed(sf::Keyboard::R)) restartLevel();
            else if (window.wasKeyPressed(sf::Keyboard::Q)) setState(GameState::MainMenu);
        }
        break;

    case GameState::LevelClear:
        if (window.wasKeyPressed(sf::Keyboard::Enter)) nextLevel();
        break;

    case GameState::GameOver:
        if (window.wasKeyPressed(sf::Keyboard::R)) restartLevel();
        else if (window.wasKeyPressed(sf::Keyboard::Enter)) setState(GameState::MainMenu);
        break;

    case GameState::Victory:
        if (window.wasKeyPressed(sf::Keyboard::Enter)) setState(GameState::MainMenu);
        break;
    }
}

void Game::update(float elapsed)
{
    hud.update(elapsed);
    if (board) board->advanceAnimation(elapsed);

    if (state != GameState::Playing) return;

    runSystems(elapsed);

    for (auto& ent : entities) {
        ent->update(this, elapsed);
    }

    handleCollisions();
    removeDeletedEntities();
    flushPendingEntities();
    checkLevelProgress();
}

void Game::handleCollisions()
{
    // Axe swing: reach box in front of the player hits logs and enemies.
    if (player->isSwingActive()) {
        const Rectangle attackBox = player->getAttackBox();
        for (auto& ent : entities) {
            if (ent->isDeleted()) continue;
            const EntityType t = ent->getEntityType();
            if (t != EntityType::LOG && t != EntityType::ENEMY) continue;
            if (!attackBox.intersects(ent->getBoundingBox())) continue;
            if (t == EntityType::LOG) player->chopLog(ent.get());
            else                      player->hitEnemy(ent.get());
        }
    }

    // Player body vs pickups / enemies.
    Rectangle& playerBB = player->getBoundingBox();
    for (auto& ent : entities) {
        if (ent == player || ent->isDeleted()) continue;
        auto it = collisionCallbacks.find(ent->getEntityType());
        if (it == collisionCallbacks.end()) continue;
        if (playerBB.intersects(ent->getBoundingBox())) {
            it->second(ent.get());
        }
    }

    // Fireballs vs enemies.
    for (auto& f : entities) {
        if (f->getEntityType() != EntityType::FIRE || f->isDeleted()) continue;
        auto fire = static_cast<Fire*>(f.get());
        for (auto& e : entities) {
            if (e->getEntityType() != EntityType::ENEMY || e->isDeleted()) continue;
            auto enemy = static_cast<Enemy*>(e.get());
            if (!enemy->isAlive()) continue;
            if (fire->getBoundingBox().intersects(enemy->getBoundingBox())) {
                enemy->takeDamage(fire->getDamage());
                fire->deleteEntity();
                break;
            }
        }
    }
    // Kills (by axe or fire) are credited in checkLevelProgress, in one place.
}

void Game::checkLevelProgress()
{
    // Damage flash on health loss.
    const int hp = player->getHealthComp()->getHealth();
    if (hp < lastPlayerHealth) hud.flashDamage();
    lastPlayerHealth = hp;

    // Count living enemies; credit every kill exactly once (score, drops).
    int alive = 0;
    for (auto& e : entities) {
        if (e->getEntityType() != EntityType::ENEMY) continue;
        auto enemy = static_cast<Enemy*>(e.get());
        if (enemy->isAlive()) {
            ++alive;
        }
        else if (!enemy->isCredited()) {
            enemy->markCredited();
            onEnemyKilled(enemy);
        }
    }
    enemiesAlive = alive;

    if (player->isDead()) {
        if (player->isDeathAnimationDone()) setState(GameState::GameOver);
        return;
    }

    if (enemiesAlive == 0) {
        if (board->hasExit()) {
            if (!board->isExitActive()) {
                board->setExitActive(true);
                hud.pushToast("Exit open! Step on the golden circle.");
            }
            if (board->isOnActiveExit(player->getBoundingBox())) {
                setState(GameState::LevelClear);
            }
        }
        else {
            levelClearDelay += 1.f / 60.f;
            if (levelClearDelay >= 1.0f) setState(GameState::LevelClear);
        }
    }
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

void Game::bigArray(float elapsed) {
    for (const auto& sys : systems) {
        for (auto& ent : entities) {
            if (sys->validate(ent.get())) {
                sys->update(this, ent.get(), elapsed);
            }
        }
    }
}

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

    // Draw order: pickups, enemies, fire, then the player on top.
    for (auto& ent : entities) {
        if (ent != player) ent->draw(&window);
    }
    if (player && state != GameState::MainMenu) player->draw(&window);

    if (state == GameState::Playing || state == GameState::Paused) {
        hud.drawGameplay(window, *this);
    }
    hud.drawOverlay(window, *this);
    window.endDraw();
}

sf::Time Game::getElapsed() const
{
    return gameClock.getElapsedTime();
}

void Game::setFPS(int fps)
{
    hud.setFPS(fps);
}

EntityID Game::getIDCounter()
{
    return entityCounter;
}
