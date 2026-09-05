#include "../../include/core/Balance.h"
#include <cctype>
#include <fstream>
#include <functional>
#include <istream>
#include <sstream>
#include <unordered_map>

Balance& Balance::get()
{
    static Balance instance;
    return instance;
}

namespace {

std::string trim(const std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

// Strict numeric parsing: the whole string must be consumed; `out` is only
// written on success.
template <typename T>
bool parseNumber(const std::string& s, T& out)
{
    std::istringstream ss(s);
    T v{};
    if (!(ss >> v)) return false;
    ss >> std::ws;
    if (!ss.eof()) return false;
    out = v;
    return true;
}

bool parseInt(const std::string& s, int& out)     { return parseNumber(s, out); }
bool parseFloat(const std::string& s, float& out) { return parseNumber(s, out); }

using Setter = std::function<bool(const std::string&)>;

Setter bindInt(int& target)     { return [&target](const std::string& v) { return parseInt(v, target); }; }
Setter bindFloat(float& target) { return [&target](const std::string& v) { return parseFloat(v, target); }; }
Setter bindString(std::string& target) { return [&target](const std::string& v) { target = v; return true; }; }

void bindEnemy(std::unordered_map<std::string, Setter>& m, const std::string& prefix, EnemyStats& e)
{
    m[prefix + ".name"]             = bindString(e.name);
    m[prefix + ".texture"]          = bindString(e.texture);
    m[prefix + ".scale"]            = bindFloat(e.scale);
    m[prefix + ".health"]           = bindInt(e.health);
    m[prefix + ".speed"]            = bindFloat(e.speed);
    m[prefix + ".contactDamage"]    = bindInt(e.contactDamage);
    m[prefix + ".chaseRange"]       = bindFloat(e.chaseRange);
    m[prefix + ".potionDropChance"] = bindFloat(e.potionDropChance);
    m[prefix + ".score"]            = bindInt(e.score);
    m[prefix + ".hitboxInset"]      = bindFloat(e.hitboxInset);
}

std::unordered_map<std::string, Setter> makeBindings(Balance& b)
{
    std::unordered_map<std::string, Setter> m;
    m["player.startingHealth"]   = bindInt(b.player.startingHealth);
    m["player.maxHealth"]        = bindInt(b.player.maxHealth);
    m["player.maxWood"]          = bindInt(b.player.maxWood);
    m["player.speed"]            = bindFloat(b.player.speed);
    m["player.shootingCost"]     = bindInt(b.player.shootingCost);
    m["player.shootCooldown"]    = bindFloat(b.player.shootCooldown);
    m["player.axeDamage"]        = bindInt(b.player.axeDamage);
    m["player.axeReach"]         = bindFloat(b.player.axeReach);
    m["player.invulnerableTime"] = bindFloat(b.player.invulnerableTime);
    m["player.inputBufferTime"]  = bindFloat(b.player.inputBufferTime);

    m["fire.speed"]    = bindFloat(b.fire.speed);
    m["fire.ttlTicks"] = bindInt(b.fire.ttlTicks);
    m["fire.damage"]   = bindInt(b.fire.damage);

    m["pickup.potionHeal"] = bindInt(b.pickup.potionHeal);
    m["pickup.logWood"]    = bindInt(b.pickup.logWood);

    bindEnemy(m, "enemy.mushroom", b.mushroom);
    bindEnemy(m, "enemy.boss", b.boss);
    return m;
}

} // namespace

bool Balance::set(const std::string& key, const std::string& value)
{
    auto bindings = makeBindings(*this);
    auto it = bindings.find(key);
    if (it == bindings.end()) return false;
    return it->second(trim(value));
}

int Balance::parse(std::istream& in, std::vector<std::string>* warnings)
{
    auto bindings = makeBindings(*this);
    std::string section;
    std::string line;
    int applied = 0;
    int lineNo = 0;

    auto warn = [&](const std::string& msg) {
        if (warnings) warnings->push_back("line " + std::to_string(lineNo) + ": " + msg);
    };

    while (std::getline(in, line)) {
        ++lineNo;
        // Strip comments (# or ;) and whitespace.
        const size_t hash = line.find_first_of("#;");
        if (hash != std::string::npos) line.erase(hash);
        line = trim(line);
        if (line.empty()) continue;

        if (line.front() == '[') {
            if (line.back() != ']') { warn("malformed section header '" + line + "'"); continue; }
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        const size_t eq = line.find('=');
        if (eq == std::string::npos) { warn("expected key = value, got '" + line + "'"); continue; }

        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));
        const std::string full = section.empty() ? key : section + "." + key;

        auto it = bindings.find(full);
        if (it == bindings.end()) { warn("unknown key '" + full + "'"); continue; }
        if (!it->second(value))   { warn("bad value '" + value + "' for '" + full + "'"); continue; }
        ++applied;
    }
    return applied;
}

bool Balance::loadFromFile(const std::string& file, std::vector<std::string>* warnings)
{
    std::ifstream in(file);
    if (!in.is_open()) return false;
    parse(in, warnings);
    return true;
}
