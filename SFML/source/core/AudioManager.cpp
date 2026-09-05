#include "../../include/core/AudioManager.h"
#include <cmath>
#include <iostream>
#include <random>

namespace {
    constexpr unsigned kRate = 44100;
    constexpr float kPi = 3.14159265358979f;

    // Small helpers to build effects out of sine/square/noise segments.
    struct Synth {
        std::vector<sf::Int16> out;

        static float env(float t, float dur, float attack = 0.005f) {
            if (t < attack) return t / attack;
            const float rel = (t - attack) / std::max(0.001f, dur - attack);
            return std::max(0.f, 1.f - rel);
        }

        void tone(float freqStart, float freqEnd, float dur, float amp, bool square = false) {
            const size_t n = static_cast<size_t>(dur * kRate);
            float phase = 0.f;
            for (size_t i = 0; i < n; ++i) {
                const float t = i / static_cast<float>(kRate);
                const float f = freqStart + (freqEnd - freqStart) * (t / dur);
                phase += 2.f * kPi * f / kRate;
                float s = std::sin(phase);
                if (square) s = s >= 0.f ? 1.f : -1.f;
                out.push_back(static_cast<sf::Int16>(s * env(t, dur) * amp * 32767.f));
            }
        }

        void noise(float dur, float amp, float lowpass = 0.3f) {
            static std::mt19937 gen{ 12345 };
            std::uniform_real_distribution<float> d(-1.f, 1.f);
            const size_t n = static_cast<size_t>(dur * kRate);
            float y = 0.f;
            for (size_t i = 0; i < n; ++i) {
                const float t = i / static_cast<float>(kRate);
                y += (d(gen) - y) * lowpass;      // one-pole low-pass for a "thud"
                out.push_back(static_cast<sf::Int16>(y * env(t, dur) * amp * 32767.f));
            }
        }

        void silence(float dur) { out.insert(out.end(), static_cast<size_t>(dur * kRate), 0); }
    };
}

AudioManager::AudioManager() = default;

void AudioManager::loadSound(const std::string& name, const std::string& filepath) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile(filepath)) {
        std::cerr << "[AudioManager] Failed to load: " << filepath << "\n";
        return;
    }
    buffers[name] = std::move(buffer);
}

void AudioManager::addSamples(const std::string& name, const std::vector<sf::Int16>& samples, unsigned sampleRate) {
    sf::SoundBuffer buffer;
    if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate)) {
        std::cerr << "[AudioManager] Failed to build synth sound: " << name << "\n";
        return;
    }
    buffers[name] = std::move(buffer);
}

void AudioManager::addSynthDefaults() {
    { Synth s; s.noise(0.12f, 0.8f, 0.25f); s.tone(180.f, 90.f, 0.15f, 0.5f);            addSamples("hurt", s.out); }
    { Synth s; s.tone(620.f, 140.f, 0.22f, 0.45f, true);                                   addSamples("enemy_die", s.out); }
    { Synth s; s.tone(220.f, 40.f, 0.6f, 0.6f, true); s.noise(0.3f, 0.7f, 0.15f);         addSamples("boss_die", s.out); }
    { Synth s; for (float f : { 523.f, 659.f, 784.f, 1047.f }) s.tone(f, f, 0.13f, 0.45f); addSamples("level_clear", s.out); }
    { Synth s; for (float f : { 392.f, 330.f, 262.f }) { s.tone(f, f * 0.97f, 0.25f, 0.5f); s.silence(0.03f); } addSamples("game_over", s.out); }
    { Synth s; s.tone(660.f, 660.f, 0.09f, 0.4f); s.tone(990.f, 990.f, 0.14f, 0.4f);      addSamples("exit_open", s.out); }
    { Synth s; s.tone(880.f, 880.f, 0.05f, 0.3f);                                          addSamples("menu", s.out); }
    { Synth s; s.noise(0.08f, 0.5f, 0.6f);                                                 addSamples("fire_hit", s.out); }
}

sf::Sound* AudioManager::acquireVoice() {
    for (auto& v : voices) {
        if (v.getStatus() != sf::Sound::Playing) return &v;
    }
    // All busy: steal the first one (oldest-ish).
    return &voices[0];
}

void AudioManager::playSound(const std::string& name, float volume, float pitch) {
    if (muted) return;
    auto it = buffers.find(name);
    if (it == buffers.end()) return;
    sf::Sound* v = acquireVoice();
    v->stop();
    v->setBuffer(it->second);
    v->setVolume(volume * masterVolume / 100.f);
    v->setPitch(pitch);
    v->play();
}

void AudioManager::setMasterVolume(float v) {
    masterVolume = std::max(0.f, std::min(100.f, v));
}

void AudioManager::setMuted(bool m) {
    muted = m;
    if (muted) for (auto& v : voices) v.stop();
}
