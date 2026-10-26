#include "CMiniaudioSystem.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

void CMiniaudioSystem::limit(float *data, std::uint64_t samples)
{
    for (std::uint64_t i = 0; i < samples; ++i)
    {
        const float v = data[i], a = std::abs(v);
        if (!std::isfinite(v))
            data[i] = 0;
        else if (a > 0.95f)
            data[i] = std::copysign(0.95f + 0.05f * (1.f - std::exp(-(a - .95f) / .05f)), v);
    }
}

void CMiniaudioSystem::callback(ma_device *device, void *output, const void *, ma_uint32 count)
{
    auto *self = static_cast<CMiniaudioSystem *>(device->pUserData);
    auto *samples = static_cast<float *>(output);
    if (ma_engine_read_pcm_frames(&self->engine, samples, count, nullptr) != MA_SUCCESS)
        std::fill(samples, samples + count * 2, 0.f);
    limit(samples, count * 2);
}

AudioVoice *CMiniaudioSystem::find(VoiceHandle id) const
{
    for (const auto &v : voices)
        if (v->id == id)
            return v.get();
    return nullptr;
}

CMiniaudioSystem::CMiniaudioSystem(AudioOutput mode) : output(mode)
{
    auto config = ma_engine_config_init();
    config.noDevice = MA_TRUE;
    config.channels = 2;
    config.sampleRate = 48000;
    if (ma_engine_init(&config, &engine) != MA_SUCCESS)
        throw std::runtime_error("Cannot initialize miniaudio mixer");
    initialized = true;
    for (auto &bus : buses)
    {
        if (ma_sound_group_init(&engine, 0, nullptr, &bus) != MA_SUCCESS)
        {
            cleanup();
            throw std::runtime_error("Cannot initialize audio bus");
        }
        ++initializedBuses;
    }
    voices.reserve(64);
    if (mode == AudioOutput::Device)
    {
        auto d = ma_device_config_init(ma_device_type_playback);
        d.playback.format = ma_format_f32;
        d.playback.channels = 2;
        d.sampleRate = 48000;
        d.dataCallback = callback;
        d.pUserData = this;
        if (ma_device_init(nullptr, &d, &device) != MA_SUCCESS)
        {
            cleanup();
            throw std::runtime_error("No audio output device is available");
        }
        deviceReady = true;
        if (ma_device_start(&device) != MA_SUCCESS)
        {
            cleanup();
            throw std::runtime_error("Cannot start audio output device");
        }
    }
}

void CMiniaudioSystem::cleanup()
{
    if (deviceReady)
    {
        ma_device_uninit(&device);
        deviceReady = false;
    }
    voices.clear();
    for (unsigned i = 0; i < initializedBuses; ++i)
        ma_sound_group_uninit(&buses[i]);
    initializedBuses = 0;
    if (initialized)
    {
        ma_engine_uninit(&engine);
        initialized = false;
    }
}

CMiniaudioSystem::~CMiniaudioSystem()
{
    if (manager)
        manager->removeResourceListener(this);
    cleanup();
}

void CMiniaudioSystem::onAttach(IResourceManager *m)
{
    if (manager && manager != m)
        throw std::logic_error("Audio system uses one resource namespace");
    manager = m;
}

void CMiniaudioSystem::onDetach(IResourceManager *m)
{
    if (manager == m)
        manager = nullptr;
}

void CMiniaudioSystem::notify(EResourceType type, ResourceId id, EListenerEvent event,
                              IResourceManager *resources)
{
    if (type != EResourceType::Audio)
        return;
    if (event == EListenerEvent::Delete)
    {
        clips.erase(id);
        return;
    }
    auto clip = resources->getAudio(id);
    if (!clip)
        throw std::runtime_error("Audio publication has no prepared clip");
    clips[id] = std::move(clip);
}

void CMiniaudioSystem::defineCue(const std::string &name, const SoundCue &cue)
{
    if (cue.clips.empty() || !cue.maxInstances || cue.maxInstances > 64 ||
        !std::isfinite(cue.gain) || cue.gain < 0 || cue.gain > 2 || cue.minDistance <= 0 ||
        cue.maxDistance <= cue.minDistance || cue.pitchVariation < 0 || cue.pitchVariation > .25f)
        throw std::runtime_error("Invalid sound cue: " + name);
    for (auto id : cue.clips)
    {
        if (!clips.contains(id))
            throw std::runtime_error("Cue references unprepared audio: " + name);
        if (cue.spatial && clips.at(id)->channels() != 1)
            throw std::runtime_error("Spatial audio must be mono");
        if (clips.at(id)->streaming() && (!cue.loop || cue.maxInstances != 1))
            throw std::runtime_error("Stream cues require one looping instance");
    }
    cues[name] = cue;
}

VoiceHandle CMiniaudioSystem::play(const std::string &name, glm::vec3 position, float fadeIn)
{
    auto entry = cues.find(name);
    if (entry == cues.end())
        return 0;
    const auto &cue = entry->second;
    unsigned count = 0;
    for (const auto &v : voices)
        if (v->cue == name && v->stopRemaining < 0)
            ++count;
    if (count >= cue.maxInstances)
        return 0;
    if (voices.size() >= 64)
    {
        auto victim =
            std::min_element(voices.begin(), voices.end(), [](const auto &a, const auto &b)
                             { return a->priority < b->priority; });
        if ((*victim)->priority > cue.priority)
            return 0;
        voices.erase(victim);
    }
    auto voice = std::make_unique<AudioVoice>();
    voice->source.asset = clips.at(cue.clips[random() % cue.clips.size()]);
    if (voice->source.asset->streaming())
        for (const auto &v : voices)
            if (v->source.asset == voice->source.asset)
            {
                // A rapid state change can return to a track while it fades out.
                // Reuse its single stream consumer instead of starting a second one.
                v->stopRemaining = -1;
                ma_sound_set_fade_in_milliseconds(&v->sound, -1, 1, 250);
                ma_sound_start(&v->sound);
                return v->id;
            }
    auto data = ma_data_source_config_init();
    data.vtable = &AudioDataSource::vtable;
    if (ma_data_source_init(&data, &voice->source.base) != MA_SUCCESS)
        throw std::runtime_error("Cannot initialize audio source");
    voice->sourceInitialized = true;
    voice->source.loop = cue.loop;
    voice->cue = name;
    voice->priority = cue.priority;
    voice->id = next++;
    voice->baseGain = cue.gain;
    if (ma_sound_init_from_data_source(&engine, &voice->source, 0,
                                       &buses[static_cast<size_t>(cue.bus)],
                                       &voice->sound) != MA_SUCCESS)
        throw std::runtime_error("Cannot initialize audio voice");
    voice->initialized = true;
    ma_sound_set_spatialization_enabled(&voice->sound, cue.spatial);
    ma_sound_set_looping(&voice->sound, cue.loop);
    ma_sound_set_doppler_factor(&voice->sound, 0);
    ma_sound_set_attenuation_model(&voice->sound, ma_attenuation_model_inverse);
    ma_sound_set_min_distance(&voice->sound, cue.minDistance);
    ma_sound_set_max_distance(&voice->sound, cue.maxDistance);
    ma_sound_set_position(&voice->sound, position.x, position.y, position.z);
    ma_sound_set_volume(&voice->sound, cue.gain);
    const float pitch = 1.f + std::uniform_real_distribution<float>(-cue.pitchVariation,
                                                                    cue.pitchVariation)(random);
    ma_sound_set_pitch(&voice->sound, pitch);
    if (fadeIn > 0)
        ma_sound_set_fade_in_milliseconds(&voice->sound, 0, 1,
                                          static_cast<ma_uint64>(fadeIn * 1000));
    ma_sound_start(&voice->sound);
    const auto id = voice->id;
    voices.push_back(std::move(voice));
    return id;
}

bool CMiniaudioSystem::playing(VoiceHandle id) const
{
    const auto *v = find(id);
    return v && ma_sound_is_playing(&v->sound) && !ma_sound_at_end(&v->sound);
}

void CMiniaudioSystem::stop(VoiceHandle id, float fade)
{
    if (auto *v = find(id))
    {
        if (fade > 0)
        {
            ma_sound_set_fade_in_milliseconds(&v->sound, -1, 0,
                                              static_cast<ma_uint64>(fade * 1000));
            v->stopRemaining = fade;
        }
        else
        {
            ma_sound_stop(&v->sound);
            v->stopRemaining = 0;
        }
    }
}

void CMiniaudioSystem::stopAll()
{
    for (const auto &v : voices)
        ma_sound_stop(&v->sound);
    voices.clear();
}

void CMiniaudioSystem::setPosition(VoiceHandle id, glm::vec3 p)
{
    if (auto *v = find(id))
        ma_sound_set_position(&v->sound, p.x, p.y, p.z);
}

void CMiniaudioSystem::setGain(VoiceHandle id, float gain)
{
    if (auto *v = find(id))
        ma_sound_set_volume(&v->sound, v->baseGain * std::clamp(gain, 0.f, 2.f));
}

void CMiniaudioSystem::setListener(glm::vec3 p, glm::vec3 f, glm::vec3 up)
{
    ma_engine_listener_set_position(&engine, 0, p.x, p.y, p.z);
    ma_engine_listener_set_direction(&engine, 0, f.x, f.y, f.z);
    ma_engine_listener_set_world_up(&engine, 0, up.x, up.y, up.z);
}

void CMiniaudioSystem::setMasterGain(float v)
{
    ma_engine_set_volume(&engine, std::clamp(v, 0.f, 1.f));
}

void CMiniaudioSystem::setBusGain(AudioBus bus, float v)
{
    ma_sound_group_set_volume(&buses[static_cast<size_t>(bus)], std::clamp(v, 0.f, 1.f));
}

void CMiniaudioSystem::update(float dt)
{
    for (const auto &v : voices)
        if (v->stopRemaining >= 0)
        {
            v->stopRemaining -= std::max(0.f, dt);
            if (v->stopRemaining <= 0)
                ma_sound_stop(&v->sound);
        }
    std::erase_if(voices, [](const auto &v)
                  { return !ma_sound_is_playing(&v->sound) || ma_sound_at_end(&v->sound); });
}

unsigned CMiniaudioSystem::activeVoices() const
{
    return static_cast<unsigned>(voices.size());
}

bool CMiniaudioSystem::audibleDevice() const
{
    return deviceReady;
}

bool CMiniaudioSystem::render(float *stereo, std::uint64_t count)
{
    if (output != AudioOutput::Offline)
        return false;
    const bool ok = ma_engine_read_pcm_frames(&engine, stereo, count, nullptr) == MA_SUCCESS;
    limit(stereo, count * 2);
    return ok;
}
