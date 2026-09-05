#pragma once
#include <functional>
#include <string>

// Gameplay events the player emits. Observers decide what to do with them.
class Observer {
public:
    virtual ~Observer() = default;
    virtual void onPotionCollected() = 0;
    virtual void onShoutPerformed() = 0;
    virtual void onWoodCollected(int amount) = 0;
    virtual void onEnemyKilled(int score) = 0;
};

// Tracks score and unlocks achievements; reports them through a callback
// (the HUD shows them as toasts) instead of printing to the console.
class AchievementObserver : public Observer {
public:
    using Notify = std::function<void(const std::string&)>;

    explicit AchievementObserver(Notify notify) : notify(std::move(notify)) {}

    // Per-level setup: how many potions the current level contains.
    void startLevel(int potionsInLevel) {
        requiredPotions = potionsInLevel;
        potionsThisLevel = 0;
    }
    void resetAll() {
        potionsThisLevel = potionsCollected = shoutsPerformed = enemiesKilled = 0;
        score = 0;
        firstBloodShown = tenKillsShown = shoutsShown = false;
    }

    void onPotionCollected() override {
        potionsCollected++;
        potionsThisLevel++;
        score += 50;
        if (requiredPotions > 0 && potionsThisLevel == requiredPotions)
            emit("Achievement: every potion on this level!");
    }

    void onShoutPerformed() override {
        shoutsPerformed++;
        if (!shoutsShown && shoutsPerformed == 5) {
            shoutsShown = true;
            emit("Achievement: shouted 5 times!");
        }
    }

    void onWoodCollected(int amount) override {
        score += amount;
    }

    void onEnemyKilled(int killScore) override {
        enemiesKilled++;
        score += killScore;
        if (!firstBloodShown) { firstBloodShown = true; emit("Achievement: first blood!"); }
        if (!tenKillsShown && enemiesKilled == 10) { tenKillsShown = true; emit("Achievement: 10 kills!"); }
    }

    int getScore() const { return score; }
    int getKills() const { return enemiesKilled; }
    int getPotions() const { return potionsCollected; }
    int getShouts() const { return shoutsPerformed; }

private:
    void emit(const std::string& msg) { if (notify) notify(msg); }

    Notify notify;
    int requiredPotions = 0;
    int potionsThisLevel = 0;
    int potionsCollected = 0;
    int shoutsPerformed = 0;
    int enemiesKilled = 0;
    int score = 0;
    bool firstBloodShown = false;
    bool tenKillsShown = false;
    bool shoutsShown = false;
};
