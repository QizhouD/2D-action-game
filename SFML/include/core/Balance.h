#pragma once
#include <iosfwd>
#include <string>
#include <vector>

// Tunable numbers for one kind of enemy.
struct EnemyStats {
    std::string name;
    std::string texture;
    float scale;            // sprite scale (texture is 50x50)
    int   health;
    float speed;            // px/s
    int   contactDamage;    // dealt to the player on touch
    float chaseRange;       // px
    float potionDropChance; // 0..1
    int   score;
    float hitboxInset;      // fraction of the sprite trimmed on each side
};

// All gameplay tuning in one place. Defaults live here so the game runs with
// no config file; config/balance.ini overrides individual keys.
//
// File format (INI-like):
//     # comment
//     [player]
//     speed = 150
//     [enemy.mushroom]
//     health = 30
struct Balance {
    struct PlayerStats {
        int   startingHealth  = 80;
        int   maxHealth       = 100;
        int   maxWood         = 999;
        float speed           = 150.f;  // px/s
        int   shootingCost    = 1;      // wood per fireball
        float shootCooldown   = 0.5f;   // seconds between fireballs
        int   axeDamage       = 15;
        float axeReach        = 48.f;   // px beyond the body hitbox
        float invulnerableTime = 1.0f;  // i-frames after being hit
        float inputBufferTime = 0.15f;  // seconds a mashed input is remembered
    } player;

    struct FireStats {
        float speed    = 240.f;  // px/s
        int   ttlTicks = 150;    // lifetime in 60 Hz simulation ticks
        int   damage   = 30;
    } fire;

    struct PickupStats {
        int potionHeal = 10;
        int logWood    = 15;
    } pickup;

    EnemyStats mushroom{ "Mushroom",      "img/mushroom50-50.png", 1.5f,  30, 70.f, 10, 400.f, 0.3f,  100, 0.12f };
    EnemyStats boss    { "Mushroom King", "img/mushroom50-50.png", 2.6f, 150, 50.f, 25, 700.f, 1.0f, 1000, 0.22f };

    static Balance& get();

    // Applies `section.key = value` lines from the stream on top of the
    // current values. Returns the number of keys applied. Unknown keys and
    // malformed lines are appended to `warnings` (if given) and skipped.
    int parse(std::istream& in, std::vector<std::string>* warnings = nullptr);

    // Convenience wrapper around parse(). Returns false if the file could not
    // be opened (defaults are kept in that case).
    bool loadFromFile(const std::string& file, std::vector<std::string>* warnings = nullptr);

    // Sets a single "section.key" to a value. Returns false if the key is
    // unknown or the value does not parse.
    bool set(const std::string& key, const std::string& value);
};
