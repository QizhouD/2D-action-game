#include "../../include/core/SaveData.h"
#include <fstream>
#include <sstream>

bool SaveData::load(const std::string& file)
{
    std::ifstream in(file);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        try {
            if (key == "highScore")           highScore = std::stoi(val);
            else if (key == "levelsUnlocked") levelsUnlocked = std::stoi(val);
            else if (key == "muted")          muted = (val == "1" || val == "true");
        }
        catch (...) { /* ignore malformed values */ }
    }
    if (levelsUnlocked < 1) levelsUnlocked = 1;
    return true;
}

bool SaveData::save(const std::string& file) const
{
    std::ofstream out(file, std::ios::trunc);
    if (!out) return false;
    out << "highScore=" << highScore << "\n"
        << "levelsUnlocked=" << levelsUnlocked << "\n"
        << "muted=" << (muted ? 1 : 0) << "\n";
    return static_cast<bool>(out);
}
