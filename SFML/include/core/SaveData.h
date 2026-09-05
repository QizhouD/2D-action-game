#pragma once
#include <string>

// Persistent progress: best score and how many levels may be started from
// the menu. Stored as plain "key=value" lines next to the executable.
struct SaveData {
    int highScore = 0;
    int levelsUnlocked = 1;
    bool muted = false;

    bool load(const std::string& file);
    bool save(const std::string& file) const;
};
