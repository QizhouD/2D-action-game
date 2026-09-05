#include "../../include/core/LevelParser.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <istream>
#include <stdexcept>

char LevelData::at(int col, int row) const
{
    if (row < 0 || row >= height || col < 0 || col >= width) return 'w';
    return rows[static_cast<size_t>(row)][static_cast<size_t>(col)];
}

int LevelData::count(char kind) const
{
    return static_cast<int>(std::count_if(spawns.begin(), spawns.end(),
                                          [kind](const Spawn& s) { return s.kind == kind; }));
}

static std::string stripCR(std::string line)
{
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

LevelData LevelParser::parse(std::istream& in, const std::string& name)
{
    LevelData lvl;
    lvl.name = name;

    std::string line;
    while (std::getline(in, line)) {
        line = stripCR(line);
        if (line.empty()) continue;
        lvl.rows.push_back(line);
    }
    if (lvl.rows.empty()) throw std::runtime_error("Level '" + name + "' is empty");

    lvl.height = static_cast<int>(lvl.rows.size());
    for (const auto& r : lvl.rows) lvl.width = std::max(lvl.width, static_cast<int>(r.size()));

    int players = 0;
    for (int row = 0; row < lvl.height; ++row) {
        std::string& r = lvl.rows[static_cast<size_t>(row)];
        r.resize(static_cast<size_t>(lvl.width), 'w');   // ragged rows become wall

        for (int col = 0; col < lvl.width; ++col) {
            const char c = r[static_cast<size_t>(col)];
            if (std::strchr(legalCells, c) == nullptr || c == '\0') {
                throw std::runtime_error("Level '" + name + "': unknown cell '" + std::string(1, c) +
                                         "' at row " + std::to_string(row) + ", col " + std::to_string(col));
            }
            switch (c) {
            case '*':
                ++players;
                lvl.playerCol = col;
                lvl.playerRow = row;
                break;
            case 'o':
                lvl.hasExit = true;
                break;
            case 'x': case 'p': case 'e': case 'B':
                lvl.spawns.push_back({ c, col, row });
                break;
            default:
                break;
            }
        }
    }

    if (players == 0) throw std::runtime_error("Level '" + name + "' has no player start ('*')");
    if (players > 1)  throw std::runtime_error("Level '" + name + "' has " + std::to_string(players) + " player starts");
    return lvl;
}

LevelData LevelParser::parseFile(const std::string& file)
{
    std::ifstream in(file);
    if (!in) throw std::runtime_error("Level file not found: " + file);
    return parse(in, file);
}

std::vector<std::string> LevelParser::parseLevelList(std::istream& in, const std::string& dir)
{
    std::vector<std::string> files;
    std::string line;
    while (std::getline(in, line)) {
        line = stripCR(line);
        // Trim surrounding whitespace.
        const size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;
        const size_t e = line.find_last_not_of(" \t");
        line = line.substr(b, e - b + 1);
        if (line[0] == '#') continue;
        files.push_back(dir + line);
    }
    return files;
}

std::vector<std::string> LevelParser::loadLevelList(const std::string& file)
{
    std::string dir;
    const size_t slash = file.find_last_of("/\\");
    if (slash != std::string::npos) dir = file.substr(0, slash + 1);

    std::ifstream in(file);
    if (!in) return {};
    return parseLevelList(in, dir);
}
