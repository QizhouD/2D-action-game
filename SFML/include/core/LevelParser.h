#pragma once
#include <iosfwd>
#include <string>
#include <vector>

// Pure-data description of a level, produced by LevelParser and consumed by
// Game::loadLevel. Has no SFML dependency so it can be unit tested.
//
// Tile legend (see levels/levels.txt):
//   w wall   . floor   o exit   * player start
//   x log    p potion  e mushroom   B boss
struct LevelData {
    struct Spawn { char kind; int col; int row; };

    std::string name;                 // file name, for error messages
    int width = 0;
    int height = 0;
    std::vector<std::string> rows;    // rectangular: short rows are padded with 'w'
    std::vector<Spawn> spawns;        // every x / p / e / B cell, in reading order
    int playerCol = -1;
    int playerRow = -1;
    bool hasExit = false;

    char at(int col, int row) const;  // 'w' outside the map
    bool isWall(int col, int row) const { return at(col, row) == 'w'; }
    int count(char kind) const;       // number of spawns of that kind
};

class LevelParser {
public:
    static constexpr const char* legalCells = "w.ox*peB";

    // Throws std::runtime_error (message includes `name`) when the level is
    // empty, contains an unknown character, or does not have exactly one '*'.
    static LevelData parse(std::istream& in, const std::string& name = "<stream>");
    static LevelData parseFile(const std::string& file);

    // A level list is one level file per line; '#' lines and blanks are
    // skipped. Entries are returned joined onto `dir` (e.g. "levels/").
    static std::vector<std::string> parseLevelList(std::istream& in, const std::string& dir);
    // Resolves `dir` from the list file's own location. Missing/empty file
    // yields an empty vector.
    static std::vector<std::string> loadLevelList(const std::string& file);
};
