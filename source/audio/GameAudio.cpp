#include "GameAudio.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <json/json.h>
#include <stdexcept>

GameAudio::GameAudio(IAudioSystem &system) : m_system(system)
{
    m_events.reserve(256);
}

GameAudio::~GameAudio()
{
    m_system.stopAll();
}

void GameAudio::configure(IResourceManager &resources)
{
    if (m_configured)
        return;
    Json::Value bank;
    if (!resources.loadJsonDocument("data/audio/cues.json", bank))
        throw std::runtime_error("Prepared audio bank is missing");

    const auto &volumes = bank["volumes"];
    m_system.setMasterGain(volumes.get("master", .6f).asFloat());
    const std::pair<const char *, AudioBus> buses[] = {{"music", AudioBus::Music},
                                                       {"effects", AudioBus::Effects},
                                                       {"engines", AudioBus::Engines},
                                                       {"ui", AudioBus::Ui}};
    for (const auto &[name, bus] : buses)
        m_system.setBusGain(bus, volumes.get(name, 1.f).asFloat());

    // The loader has already parsed this document and prepared every variation.
    for (const auto &name : bank["cues"].getMemberNames())
    {
        const auto &node = bank["cues"][name];
        SoundCue cue;
        const auto busName = node.get("bus", "effects").asString();
        bool found = false;
        for (const auto &[key, bus] : buses)
            if (busName == key)
            {
                cue.bus = bus;
                found = true;
            }
        if (!found)
            throw std::runtime_error("Unknown audio bus: " + busName);
        cue.gain = node.get("gain", 1.f).asFloat();
        cue.pitchVariation = node.get("pitch_variation", 0.f).asFloat();
        cue.minDistance = node.get("min_distance", 10.f).asFloat();
        cue.maxDistance = node.get("max_distance", 900.f).asFloat();
        cue.loop = node.get("loop", false).asBool();
        cue.spatial = node.get("spatial", false).asBool();
        cue.maxInstances = node.get("max_instances", 4).asUInt();
        cue.priority = node.get("priority", 0).asInt();
        for (const auto &path : node["files"])
            cue.clips.push_back(resources.loadAudio(
                path.asString(), node.get("stream", false).asBool(), cue.spatial ? 1 : 2));
        m_system.defineCue(name, cue);
    }
    m_configured = true;
}

void GameAudio::enterState(const std::string &state)
{
    if (!m_configured)
        return;
    stopEmitters();
    if (state == "cinematic")
    {
        // Stop in-flight gun/hit one-shots too, and discard events queued earlier in this step.
        m_system.stopAll();
        m_events.clear();
        m_music = 0;
    }
    const std::string music = state == "game"    ? "combat_music"
                              : state == "title" ? "title_music"
                                                 : "";
    if (music != m_musicCue)
    {
        m_system.stop(m_music, 1.f);
        m_music = music.empty() ? 0 : m_system.play(music, {}, 1.f);
        m_musicCue = music;
    }
    if (state == "title" || state == "game")
        m_system.play("confirm");
    if (state == "win")
        m_system.play("victory");
    if (state == "lose")
        m_system.play("defeat");
}

void GameAudio::emit(GameAudioEvent event)
{
    if (m_events.size() < 256)
        m_events.push_back(event);
}

void GameAudio::update(float seconds)
{
    // Events are collected during game-world updates and played once per frame.
    for (const auto &event : m_events)
    {
        const char *cue = event.type == GameAudioEventType::MothershipCharge ? "mothership_charge"
                          : event.type == GameAudioEventType::MothershipBeam ? "mothership_beam"
                          : event.type == GameAudioEventType::PlayerHit      ? "player_hit"
                          : event.type == GameAudioEventType::Shot           ? "shot"
                          : event.type == GameAudioEventType::Hit            ? "hit"
                                                                             : "explosion";
        m_system.play(cue, event.position);
    }
    m_events.clear();
    m_system.update(seconds);
}

VoiceHandle GameAudio::startEmitter(const std::string &cue, glm::vec3 position)
{
    const auto voice = m_system.play(cue, position, .25f);
    if (voice)
        m_emitters.push_back(voice);
    return voice;
}

bool GameAudio::emitterActive(VoiceHandle handle) const
{
    return handle && m_system.playing(handle);
}

void GameAudio::moveEmitter(VoiceHandle handle, glm::vec3 position, float gain, float pitch)
{
    m_system.setPosition(handle, position);
    m_system.setGain(handle, gain);
    m_system.setPitch(handle, pitch);
}

void GameAudio::stopEmitter(VoiceHandle handle)
{
    m_system.stop(handle, .08f);
    std::erase(m_emitters, handle);
}

void GameAudio::stopEmitters()
{
    for (auto handle : m_emitters)
        m_system.stop(handle, .15f);
    m_emitters.clear();
}

void GameAudio::setListener(glm::vec3 position, glm::vec3 forward, glm::vec3 up)
{
    m_system.setListener(position, forward, up);
}
