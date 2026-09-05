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
#include "../../include/core/Balance.h"
#include "../../include/core/LevelParser.h"

void Game::registerCollisionCallback(EntityType type, std::function<void(Entity*)> callback) {
    collisionCallbacks[type] = callback;
}

std::shared_ptr<AudioManager> ServiceLocator::audioService = nullptr;

Game::Game()
    : state(GameState::MainMenu)
    , currentLevel(0)
    , enemiesAlive(0)
    , lastPlayerHealth(0)
    , levelClearDelay(0.f)
    , entityCounter(1)
    , rng(std::random_device{}())
{
    inputHandler = std::make_unique<InputHandler>();

    // Order matters: input decides direction, AI decides direction, movement
    // integrates, TTL expires projectiles.
    systems.push_back(std::make_shared<InputSystem>());
    systems.push_back(std::make_shared<AISystem>());
    systems.push_back(std::make_shared<MovementSystem>());
    systems.push_back(std::make_shared<TTLSystem>());
}

Game::~Game() {}

// ---------------------------------------------------------------------------
// Boot / assets
// ---------------------------------------------------------------------------

void Game::init(const std::string& levelListFile)
{
    // Tuning overrides. Missing file = built-in defaults; bad lines are reported.
    {
        std::vector<std::string> warnings;
        if (Balance::get().loadFromFile("config/balance.ini", &warnings)) {
            for (const auto& w : warnings) std::cerr << "balance.ini " << w << "\n";
        }
    }

    window.loadFont("font/AmaticSC-Regular.ttf");
    window.setTitle("Dwarf & Fire");
    hud.init(window.getGUIFont(), "img/log.png");

    save.load(saveFile);

    auto audio = std::make_shared<AudioManager>();
    audio->loadSound("pickup", "audio/potion_collect.wav");
    audio->loadSound("fire", "audio/fire.wav");
    audio->loadSound("axe", "audio/sword-slash.wav");
    audio->addSynthDefaults();
    audio->setMuted(save.muted);
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
    if (save.levelsUnlocked > getLevelCount()) save.levelsUnlocked = getLevelCount();
    menuLevel = 0;
    loadLevel(0);                 // shown behind the main menu
    setState(GameState::MainMenu);
}

void Game::playSound(const std::string& name, float volume, float pitch)
{
    if (auto audio = ServiceLocator::getAudio()) audio->playSound(name, volume, pitch);
}

void Game::toggleMute()
{
    save.muted = !save.muted;
    if (auto audio = ServiceLocator::getAudio()) audio->setMuted(save.muted);
    save.save(saveFile);
    hud.pushToast(save.muted ? "Sound off" : "Sound on", 1.2f);
}

void Game::recordProgress()
{
    bool changed = false;
    if (getScore() > save.highScore) { save.highScore = getScore(); changed = true; }
    const int reached = std::min(getLevelCount(), currentLevel + 2);   // next level becomes startable
    if (state == GameState::LevelClear && reached > save.levelsUnlocked) { save.levelsUnlocked = reached; changed = true; }
    if (changed) save.save(saveFile);
}

void Game::loadLevelList(const std::string& file)
{
    levelFiles = LevelParser::loadLevelList(file);
    if (levelFiles.empty()) {
        std::cerr << "No levels listed in " << file << ", falling back to levels/lvl0.txt\n";
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

    // Parsing/validation is separate from world construction so it can be
    // unit tested and so a broken level fails before we tear down the old one.
    const LevelData lvl = LevelParser::parseFile(levelFiles[index]);

    // Reset world state (the player object survives).
    entities.clear();
    pendingEntities.clear();
    hud.clearToasts();
    enemiesAlive = 0;
    levelClearDelay = 0.f;

    const float tile = spriteWH * tileScale;
    board = std::make_unique<Board>(lvl.width, lvl.height, tile);
    particles.clear();

    const sf::Vector2u worldPx(static_cast<unsigned>(lvl.width * tile), static_cast<unsigned>(lvl.height * tile));
    window.setWorld(worldPx);
    if (!window.isCreated()) window.setup("Dwarf & Fire", window.getLogicalSize());

    for (int row = 0; row < lvl.height; ++row) {
        for (int col = 0; col < lvl.width; ++col) {
            switch (lvl.at(col, row)) {
            case 'w': board->addTile(col, row, tileScale, TileType::WALL,     "img/wall.png");  break;
            case 'o': board->addTile(col, row, tileScale, TileType::EXIT,     "img/floor.png"); break;
            default:  board->addTile(col, row, tileScale, TileType::CORRIDOR, "img/floor.png"); break;
            }
        }
    }

    for (const auto& s : lvl.spawns) {
        switch (s.kind) {
        case 'x':
            addEntity(buildEntityAt<Log>("img/log.png", s.col, s.row));
            break;
        case 'p':
            addEntity(buildEntityAt<Potion>("img/potion.png", s.col, s.row));
            break;
        case 'e':
        case 'B': {
            const EnemyStats& st = (s.kind == 'B') ? Balance::get().boss : Balance::get().mushroom;
            auto enemy = std::make_shared<Enemy>(st);
            enemy->init(st.texture, st.scale);
            placeInTile(*enemy, s.col, s.row);
            addEntity(enemy);
            ++enemiesAlive;
            break;
        }
        default:
            break;
        }
    }

    player->positionSprite(lvl.playerRow, lvl.playerCol, spriteWH, tileScale);
    player->onLevelStart();
    addEntity(player);
    flushPendingEntities();
    window.snapCamera(player->getCenter());

    achievementObserver->startLevel(lvl.count('p'));
    lastPlayerHealth = player->getHealthComp()->getHealth();
    board->setExitActive(enemiesAlive == 0);
}

void Game::startNewGame(int firstLevel)
{
    achievementObserver->resetAll();
    player->resetStats();
    loadLevel(std::max(0, std::min(firstLevel, getLevelCount() - 1)));
    setState(GameState::Playing);
    hud.pushToast(currentLevel == 0 ? "Level 1: clear the mushrooms!"
                                    : "Level " + std::to_string(currentLevel + 1));
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
    if (s == state) return;
    state = s;
    switch (state) {
    case GameState::LevelClear: playSound("level_clear"); recordProgress(); break;
    case GameState::Victory:    playSound("level_clear", 100.f, 1.2f); recordProgress(); break;
    case GameState::GameOver:   playSound("game_over"); recordProgress(); break;
    case GameState::Paused:     playSound("menu", 60.f, 0.8f); break;
    default: break;
    }
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
    for (auto& newEntity : pendingEntities)
        entities.push_back(std::move(newEntity));
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

    const bool boss = enemy->getStats().score >= 1000;
    playSound(boss ? "boss_die" : "enemy_die");
    const float size = enemy->getSpriteSize().x;
    particles.burst(enemy->getCenter(), boss ? 90 : 26, size * 1.6f, boss ? 0.9f : 0.5f, size * 0.09f,
                    sf::Color(255, 90, 90), sf::Color(120, 20, 20, 0));
    particles.burst(enemy->getCenter(), boss ? 40 : 12, size * 0.8f, 0.6f, size * 0.06f,
                    sf::Color(250, 250, 230), sf::Color(250, 250, 230, 0));
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Game::handleInput()
{
    window.pollEvents();

    if (window.wasKeyPressed(sf::Keyboard::F1)) window.setDebugDraw(!window.isDebugDraw());
    if (window.wasKeyPressed(sf::Keyboard::M))  toggleMute();

    // Gamepad: A confirms, Start pauses/confirms, B/Back goes back.
    const bool confirm = window.wasKeyPressed(sf::Keyboard::Enter) || window.wasJoyButtonPressed(0) || window.wasJoyButtonPressed(7);
    const bool back    = window.wasKeyPressed(sf::Keyboard::Escape) || window.wasJoyButtonPressed(1) || window.wasJoyButtonPressed(6);
    const bool left    = window.wasKeyPressed(sf::Keyboard::Left)  || window.wasKeyPressed(sf::Keyboard::A);
    const bool right   = window.wasKeyPressed(sf::Keyboard::Right) || window.wasKeyPressed(sf::Keyboard::D);

    // Auto-pause when the window goes to the background.
    if (state == GameState::Playing && !window.hasFocus()) setState(GameState::Paused);

    switch (state) {
    case GameState::MainMenu:
        if (confirm) { playSound("menu"); startNewGame(menuLevel); }
        else if (back) window.close();
        else if (left && menuLevel > 0)  { --menuLevel; playSound("menu", 60.f); }
        else if (right && menuLevel + 1 < save.levelsUnlocked) { ++menuLevel; playSound("menu", 60.f); }
        break;

    case GameState::Playing:
    case GameState::Paused:
        if (auto cmd = inputHandler->handleInput(window)) cmd->execute(*this);
        if (state == GameState::Paused) {
            if (window.wasKeyPressed(sf::Keyboard::R)) restartLevel();
            else if (window.wasKeyPressed(sf::Keyboard::Q) || window.wasJoyButtonPressed(6)) setState(GameState::MainMenu);
        }
        break;

    case GameState::LevelClear:
        if (confirm) nextLevel();
        break;

    case GameState::GameOver:
        if (window.wasKeyPressed(sf::Keyboard::R) || window.wasJoyButtonPressed(0)) restartLevel();
        else if (window.wasKeyPressed(sf::Keyboard::Enter) || window.wasJoyButtonPressed(1)) setState(GameState::MainMenu);
        break;

    case GameState::Victory:
        if (confirm) setState(GameState::MainMenu);
        break;
    }
}

void Game::update(float elapsed)
{
    hud.update(elapsed);
    if (board) board->advanceAnimation(elapsed);
    if (state != GameState::Paused) particles.update(elapsed);

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
                playSound("fire_hit", 80.f);
                particles.burst(fire->getCenter(), 14, 140.f, 0.3f, 5.f,
                                sf::Color(255, 220, 120), sf::Color(200, 60, 0, 0));
                break;
            }
        }
    }
    // Kills (by axe or fire) are credited in checkLevelProgress, in one place.
}

void Game::checkLevelProgress()
{
    // Damage feedback on health loss.
    const int hp = player->getHealthComp()->getHealth();
    if (hp < lastPlayerHealth) {
        hud.flashDamage();
        playSound("hurt");
        particles.burst(player->getCenter(), 14, 120.f, 0.4f, 4.f,
                        sf::Color(255, 60, 60), sf::Color(160, 0, 0, 0));
    }
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
                playSound("exit_open");
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
    // System-major order keeps each system's logic cache-friendly and makes
    // the pipeline order (input -> AI -> movement -> TTL) explicit.
    for (const auto& sys : systems) {
        for (auto& ent : entities) {
            if (sys->validate(ent.get()))
                sys->update(this, ent.get(), elapsed);
        }
    }
}

void Game::removeDeletedEntities()
{
    auto isDeleted = [](const std::shared_ptr<Entity>& e) { return e->isDeleted(); };
    entities.erase(std::remove_if(entities.begin(), entities.end(), isDeleted), entities.end());
}

void Game::render(float elapsed)
{
    if (player) window.followCamera(player->getCenter(), elapsed);

    window.beginDraw();

    // World layer (scrolls with the camera).
    window.beginWorld();
    if (board) { board->draw(&window); }
    // Draw order: pickups, enemies, fire, then the player on top.
    for (auto& ent : entities) {
        if (ent != player) ent->draw(&window);
    }
    if (player && state != GameState::MainMenu) player->draw(&window);
    particles.draw(window);

    // UI layer (fixed to the screen).
    window.beginUI();
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
