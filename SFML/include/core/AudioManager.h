#pragma once
#include <SFML/Audio.hpp>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Sound effects with a small voice pool so the same effect can overlap
// (e.g. two mushrooms dying in the same frame). Effects come either from
// files or from tiny procedural generators (see addSynth*), so the game
// does not depend on audio assets it does not ship.
class AudioManager {
public:
    AudioManager();

    void loadSound(const std::string& name, const std::string& filepath);
    // Registers a buffer built from raw 16-bit mono samples.
    void addSamples(const std::string& name, const std::vector<sf::Int16>& samples, unsigned sampleRate = 44100);

    // Built-in procedural effects (hurt, enemy_die, level_clear, game_over,
    // exit_open, menu, boss_die). Cheap to generate; called once at start-up.
    void addSynthDefaults();

    void playSound(const std::string& name, float volume = 100.f, float pitch = 1.f);

    void setMasterVolume(float v);       // 0..100
    float getMasterVolume() const { return masterVolume; }
    void setMuted(bool m);
    bool isMuted() const { return muted; }

private:
    sf::Sound* acquireVoice();

    std::map<std::string, sf::SoundBuffer> buffers;
    std::array<sf::Sound, 16> voices;
    float masterVolume = 70.f;
    bool muted = false;
};
