// Minimal self-contained unit tests for the SFML-free core (no framework
// needed). Run via `ctest --test-dir build` or execute minigame_tests directly.
#include "include/core/Balance.h"
#include "include/core/LevelParser.h"
#include "include/core/SaveData.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                        \
    do {                                                                                   \
        ++g_checks;                                                                        \
        if (!(cond)) {                                                                     \
            ++g_failures;                                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n";     \
        }                                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        ++g_checks;                                                                        \
        if (!((a) == (b))) {                                                               \
            ++g_failures;                                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK_EQ failed: " #a " == " #b \
                      << "  (" << (a) << " vs " << (b) << ")\n";                           \
        }                                                                                  \
    } while (0)

#define CHECK_THROWS(expr, needle)                                                         \
    do {                                                                                   \
        ++g_checks;                                                                        \
        bool thrown = false;                                                               \
        try { (void)(expr); }                                                              \
        catch (const std::exception& e) {                                                  \
            thrown = std::string(e.what()).find(needle) != std::string::npos;              \
            if (!thrown) std::cerr << "  unexpected message: " << e.what() << "\n";        \
        }                                                                                  \
        if (!thrown) {                                                                     \
            ++g_failures;                                                                  \
            std::cerr << __FILE__ << ":" << __LINE__ << ": expected throw containing \""   \
                      << needle << "\" from " #expr "\n";                                  \
        }                                                                                  \
    } while (0)

bool near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) < eps; }

struct TestCase { const char* name; std::function<void()> fn; };
std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
struct Register { Register(const char* n, std::function<void()> f) { registry().push_back({ n, std::move(f) }); } };
#define TEST(name) \
    void name(); \
    Register reg_##name(#name, name); \
    void name()

// ---------------------------------------------------------------------------
// LevelParser
// ---------------------------------------------------------------------------

TEST(level_parse_basic)
{
    std::istringstream in(
        "wwwww\n"
        "w*.xw\n"
        "wp.ew\n"
        "w..ow\n"
        "wwwww\n");
    const LevelData lvl = LevelParser::parse(in, "t");
    CHECK_EQ(lvl.width, 5);
    CHECK_EQ(lvl.height, 5);
    CHECK_EQ(lvl.playerCol, 1);
    CHECK_EQ(lvl.playerRow, 1);
    CHECK(lvl.hasExit);
    CHECK_EQ(lvl.count('x'), 1);
    CHECK_EQ(lvl.count('p'), 1);
    CHECK_EQ(lvl.count('e'), 1);
    CHECK_EQ(lvl.count('B'), 0);
    CHECK_EQ(lvl.spawns.size(), static_cast<size_t>(3));
    CHECK(lvl.isWall(0, 0));
    CHECK(!lvl.isWall(2, 2));
    CHECK_EQ(lvl.at(3, 3), 'o');
}

TEST(level_parse_ragged_rows_and_crlf)
{
    // Short rows are padded with wall; CRLF endings and blank lines are ignored.
    std::istringstream in("www\r\n\r\nw*\r\nwww\r\n");
    const LevelData lvl = LevelParser::parse(in, "t");
    CHECK_EQ(lvl.width, 3);
    CHECK_EQ(lvl.height, 3);
    CHECK_EQ(lvl.at(2, 1), 'w');
    CHECK_EQ(lvl.rows[1], std::string("w*w"));
    // Outside the map counts as wall.
    CHECK(lvl.isWall(-1, 0));
    CHECK(lvl.isWall(0, 99));
}

TEST(level_parse_errors)
{
    std::istringstream empty("\n\n");
    CHECK_THROWS(LevelParser::parse(empty, "empty.txt"), "empty");

    std::istringstream noPlayer("www\nw.w\nwww\n");
    CHECK_THROWS(LevelParser::parse(noPlayer, "np.txt"), "no player start");

    std::istringstream twoPlayers("w*w\nw*w\n");
    CHECK_THROWS(LevelParser::parse(twoPlayers, "tp.txt"), "2 player starts");

    std::istringstream badChar("w*w\nwZw\n");
    CHECK_THROWS(LevelParser::parse(badChar, "bc.txt"), "unknown cell 'Z'");
}

TEST(level_list_parsing)
{
    std::istringstream in(
        "# comment\n"
        "\n"
        "  lvl0.txt  \r\n"
        "lvl1.txt\n"
        "   # indented comment\n");
    const auto files = LevelParser::parseLevelList(in, "levels/");
    CHECK_EQ(files.size(), static_cast<size_t>(2));
    CHECK_EQ(files[0], std::string("levels/lvl0.txt"));
    CHECK_EQ(files[1], std::string("levels/lvl1.txt"));

    CHECK(LevelParser::loadLevelList("does/not/exist.txt").empty());
}

TEST(shipped_levels_are_valid)
{
    // Every level referenced by the real levels.txt must parse, have a start
    // and (so the game can be finished) either an exit or at least one enemy.
    const std::string listFile = std::string(MINIGAME_LEVELS_DIR) + "/levels.txt";
    const auto files = LevelParser::loadLevelList(listFile);
    CHECK(files.size() >= 3);
    for (const auto& f : files) {
        LevelData lvl;
        try { lvl = LevelParser::parseFile(f); }
        catch (const std::exception& e) { ++g_failures; std::cerr << e.what() << "\n"; continue; }
        CHECK(lvl.playerCol >= 0);
        CHECK(lvl.hasExit || lvl.count('e') + lvl.count('B') > 0);
        // The border must be solid so nothing can leave the map.
        for (int c = 0; c < lvl.width; ++c) { CHECK(lvl.isWall(c, 0)); CHECK(lvl.isWall(c, lvl.height - 1)); }
        for (int r = 0; r < lvl.height; ++r) { CHECK(lvl.isWall(0, r)); CHECK(lvl.isWall(lvl.width - 1, r)); }
    }
}

// ---------------------------------------------------------------------------
// Balance
// ---------------------------------------------------------------------------

TEST(balance_defaults)
{
    Balance b;
    CHECK_EQ(b.player.startingHealth, 80);
    CHECK_EQ(b.player.maxHealth, 100);
    CHECK(near(b.player.speed, 150.f));
    CHECK_EQ(b.fire.damage, 30);
    CHECK_EQ(b.pickup.logWood, 15);
    CHECK_EQ(b.mushroom.health, 30);
    CHECK_EQ(b.boss.score, 1000);
}

TEST(balance_parse_ini)
{
    Balance b;
    std::istringstream in(
        "# global comment\n"
        "[player]\n"
        "speed = 200   ; trailing comment\n"
        "axeDamage=99\n"
        "\n"
        "[fire]\n"
        "ttlTicks = 30\n"
        "[enemy.boss]\n"
        "name = Big Shroom\n"
        "health = 500\n"
        "potionDropChance = 0.25\n");
    std::vector<std::string> warnings;
    const int applied = b.parse(in, &warnings);
    CHECK_EQ(applied, 6);
    CHECK(warnings.empty());
    CHECK(near(b.player.speed, 200.f));
    CHECK_EQ(b.player.axeDamage, 99);
    CHECK_EQ(b.fire.ttlTicks, 30);
    CHECK_EQ(b.boss.name, std::string("Big Shroom"));
    CHECK_EQ(b.boss.health, 500);
    CHECK(near(b.boss.potionDropChance, 0.25f));
    // Untouched keys keep their defaults.
    CHECK_EQ(b.player.maxHealth, 100);
    CHECK_EQ(b.mushroom.health, 30);
}

TEST(balance_parse_reports_bad_lines)
{
    Balance b;
    std::istringstream in(
        "[player]\n"
        "speed = fast\n"          // bad value
        "jumpHeight = 3\n"        // unknown key
        "this line has no equals\n"
        "[broken\n"               // malformed header
        "maxWood = 5\n");         // still applies (section stays 'player')
    std::vector<std::string> warnings;
    const int applied = b.parse(in, &warnings);
    CHECK_EQ(applied, 1);
    CHECK_EQ(warnings.size(), static_cast<size_t>(4));
    CHECK(near(b.player.speed, 150.f));   // bad value must not clobber the default
    CHECK_EQ(b.player.maxWood, 5);
    CHECK(warnings[0].find("line 2") != std::string::npos);
    CHECK(warnings[1].find("unknown key 'player.jumpHeight'") != std::string::npos);
}

TEST(balance_set_and_missing_file)
{
    Balance b;
    CHECK(b.set("pickup.potionHeal", " 42 "));
    CHECK_EQ(b.pickup.potionHeal, 42);
    CHECK(!b.set("pickup.potionHeal", "12abc"));
    CHECK_EQ(b.pickup.potionHeal, 42);
    CHECK(!b.set("nope.nothing", "1"));
    CHECK(!b.loadFromFile("no/such/balance.ini"));
}

TEST(shipped_balance_ini_matches_defaults)
{
    // The checked-in config documents the defaults; loading it must not
    // silently change gameplay and must not produce warnings.
    Balance fromFile;
    std::vector<std::string> warnings;
    CHECK(fromFile.loadFromFile(std::string(MINIGAME_CONFIG_DIR) + "/balance.ini", &warnings));
    for (const auto& w : warnings) std::cerr << "balance.ini " << w << "\n";
    CHECK(warnings.empty());

    const Balance def;
    CHECK_EQ(fromFile.player.startingHealth, def.player.startingHealth);
    CHECK(near(fromFile.player.speed, def.player.speed));
    CHECK_EQ(fromFile.player.axeDamage, def.player.axeDamage);
    CHECK(near(fromFile.fire.speed, def.fire.speed));
    CHECK_EQ(fromFile.fire.ttlTicks, def.fire.ttlTicks);
    CHECK_EQ(fromFile.pickup.potionHeal, def.pickup.potionHeal);
    CHECK_EQ(fromFile.mushroom.name, def.mushroom.name);
    CHECK_EQ(fromFile.mushroom.health, def.mushroom.health);
    CHECK_EQ(fromFile.boss.name, def.boss.name);
    CHECK(near(fromFile.boss.hitboxInset, def.boss.hitboxInset));
}

// ---------------------------------------------------------------------------
// SaveData
// ---------------------------------------------------------------------------

TEST(savedata_roundtrip_and_tolerance)
{
    const auto path = std::filesystem::temp_directory_path() / "minigame_test_save.txt";
    const std::string file = path.string();

    SaveData a;
    a.highScore = 1234;
    a.levelsUnlocked = 3;
    a.muted = true;
    CHECK(a.save(file));

    SaveData b;
    CHECK(b.load(file));
    CHECK_EQ(b.highScore, 1234);
    CHECK_EQ(b.levelsUnlocked, 3);
    CHECK(b.muted);

    // Garbage values are ignored and levelsUnlocked is clamped to >= 1.
    {
        std::ofstream out(file, std::ios::trunc);
        out << "highScore=notanumber\nlevelsUnlocked=0\nmuted=true\nunknown=1\n";
    }
    SaveData c;
    CHECK(c.load(file));
    CHECK_EQ(c.highScore, 0);
    CHECK_EQ(c.levelsUnlocked, 1);
    CHECK(c.muted);

    std::filesystem::remove(path);
    SaveData d;
    CHECK(!d.load(file));
}

} // namespace

int main()
{
    for (const auto& t : registry()) {
        const int before = g_failures;
        try { t.fn(); }
        catch (const std::exception& e) { ++g_failures; std::cerr << "  exception: " << e.what() << "\n"; }
        std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << t.name << "\n";
    }
    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
