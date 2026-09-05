#pragma once
#include "../../include/graphics/Window.h"
#include "../../include/graphics/Hud.h"
#include "../../include/core/Board.h"
#include "../../include/entities/Player.h"
#include "Command.h"
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <SFML/System/Time.hpp>
#include "../../include/systems/Systems.h"
#include "../../include/utils/PackedArray.h"
#include "../../include/utils/Observer.h"
#include <unordered_map>
#include <functional> 

class InputHandler;
class Player;
class Entity;
class Enemy;
class System;

using EntityID = unsigned int;
enum class ECSType { BIG_ARRAY, ARCHETYPES, PACKED_ARRAY };
enum class GameState { MainMenu, Playing, Paused, LevelClear, GameOver, Victory };

class Archetype {
public:
    std::vector<std::shared_ptr<Entity>> entities; // Entities sharing the same component structure
    Bitmask componentMask;                         // Bitmask representing common components
};

class Game {
public:
    const int spriteWH = 50;
    const float tileScale = 2.0f;
    const float itemScale = 1.0f;

    void registerCollisionCallback(EntityType type, std::function<void(Entity*)> callback);

    Game(ECSType type = ECSType::BIG_ARRAY);
    ~Game();

    // Loads assets and the level list, shows the main menu.
    void init(const std::string& levelListFile = "levels/levels.txt");
    void addEntity(std::shared_ptr<Entity> newEntity);

    void handleInput();
    void update(float elapsed);
    void render(float elapsed);
    Window* getWindow() { return &window; }
    Board* getBoard() const { return board.get(); }

    sf::Time getElapsed() const;
    void setFPS(int FPS);

    // --- State -------------------------------------------------------------
    GameState getState() const { return state; }
    void togglePause();
    bool isPaused() const { return state == GameState::Paused; }

    std::shared_ptr<Player> getPlayer() const { return player; }
    int getEnemiesAlive() const { return enemiesAlive; }
    int getLevelIndex() const { return currentLevel; }
    int getLevelCount() const { return static_cast<int>(levelFiles.size()); }
    int getScore() const { return achievementObserver ? achievementObserver->getScore() : 0; }
    int getKills() const { return achievementObserver ? achievementObserver->getKills() : 0; }

    void pushToast(const std::string& text) { hud.pushToast(text); }

    // Gameplay events
    void onEnemyKilled(Enemy* enemy);
    void spawnPotionAt(const sf::Vector2f& center);

    EntityID getIDCounter();

    template <typename T>
    std::shared_ptr<T> buildEntityAt(const std::string& filename, int col, int row)
    {
        auto ent = std::make_shared<T>();
        float x = col * spriteWH * tileScale;
        float y = row * spriteWH * tileScale;
        float cntrFactor = (tileScale - itemScale) * spriteWH * 0.5f;
        ent->setPosition(x + cntrFactor, y + cntrFactor);
        ent->init(filename, itemScale);
        return ent;
    }

private:
    // --- Level flow --------------------------------------------------------
    void loadLevelList(const std::string& file);
    void loadLevel(int index);
    void startNewGame();
    void restartLevel();
    void nextLevel();
    void setState(GameState s);
    // Centres an already-initialised entity inside tile (col,row).
    void placeInTile(Entity& ent, int col, int row) const;

    // --- Simulation --------------------------------------------------------
    void runSystems(float elapsed);
    void updateArchetypes(float elapsed);
    void bigArray(float elapsed);
    void updatePackedArray(float elapsed);
    void handleCollisions();
    void checkLevelProgress();
    void removeDeletedEntities();
    // Entities spawned during a frame are queued and inserted here, so that
    // spawning never invalidates a container that is being iterated.
    void flushPendingEntities();

    Window window;
    Hud hud;
    GameState state;
    sf::Clock gameClock;
    sf::Time elapsed;

    std::vector<std::string> levelFiles;
    int currentLevel;
    int enemiesAlive;
    int lastPlayerHealth;
    float levelClearDelay;   // small pause between last kill and LevelClear when there is no exit

    std::unique_ptr<Board> board;
    std::vector<std::shared_ptr<Entity>> entities;
    std::vector<std::shared_ptr<Entity>> pendingEntities;
    std::vector<std::shared_ptr<System>> systems;
    EntityID entityCounter;
    std::shared_ptr<Player> player;
    std::unique_ptr<InputHandler> inputHandler;
    std::vector<std::shared_ptr<System>> graphicsSystems;
    //variables for ECS architecture selection
    ECSType ecsType;
    std::vector<Archetype> archetypes;  // For Archetypes ECS
    PackedArray<Entity> packedEntities; // Packed storage

    std::mt19937 rng;

    // Added Observer Pattern support
    std::shared_ptr<AchievementObserver> achievementObserver;
    std::unordered_map<EntityType, std::function<void(Entity*)>> collisionCallbacks;
};
