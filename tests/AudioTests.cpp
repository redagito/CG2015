#include "audio/AudioAsset.h"
#include "audio/GameAudio.h"
#include "audio/IAudioSystem.h"
#include "collision/CCollidable.h"
#include "debug/Logging.h"
#include "game/CGameObject.h"
#include "game/control/CAudioEmitterController.h"
#include "game/control/CHealthController.h"
#include "resource/Resource.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{
void writeTone(const std::string &file)
{
    std::ofstream out(file, std::ios::binary);
    const auto word = [&](std::uint16_t value)
    { out.write(reinterpret_cast<const char *>(&value), 2); };
    const auto integer = [&](std::uint32_t value)
    { out.write(reinterpret_cast<const char *>(&value), 4); };
    out.write("RIFF", 4);
    integer(36 + 96000);
    out.write("WAVEfmt ", 8);
    integer(16);
    word(1);
    word(1);
    integer(48000);
    integer(96000);
    word(2);
    word(16);
    out.write("data", 4);
    integer(96000);
    for (int i = 0; i < 48000; ++i)
    {
        const auto sample = std::int16_t(8000 * std::sin(2 * 3.141592653589793 * 440 * i / 48000));
        out.write(reinterpret_cast<const char *>(&sample), 2);
    }
}
unsigned crossings(const std::vector<float> &stereo)
{
    unsigned count = 0;
    for (std::size_t frame = 1; frame < stereo.size() / 2; ++frame)
        if (stereo[(frame - 1) * 2] <= 0 && stereo[frame * 2] > 0)
            ++count;
    return count;
}
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

AsyncLoadStatus waitFor(IResourceManager &resources, AsyncLoadId request, bool publish = true)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    for (;;)
    {
        if (publish)
            resources.pumpAsyncLoads(1.0);
        const auto status = resources.getLoadStatus(request);
        if (status.terminal() || (!publish && status.phase == LoadPhase::AwaitingUpload))
            return status;
        require(std::chrono::steady_clock::now() < deadline, "Audio load timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

double energy(const std::vector<float> &samples, unsigned channel = 2)
{
    double sum = 0;
    for (size_t i = 0; i < samples.size(); ++i)
        if (channel == 2 || i % 2 == channel)
            sum += samples[i] * samples[i];
    return sum / std::max<size_t>(1, samples.size() / 2);
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        logging::initialize();
        if (argc > 1 && std::string(argv[1]) == "--device-check")
        {
            auto device = createAudioSystem(AudioOutput::Device);
            require(device->audibleDevice(), "No physical output device could be initialized");
            std::cout << "Playback device initialized successfully (no sound played).\n";
            return 0;
        }
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        auto audio = createAudioSystem(AudioOutput::Offline);
        resources->addResourceListener(audio.get());
        require(!audio->audibleDevice(), "Offline test opened a physical audio device");

        std::filesystem::create_directories("build/audio-tests");
        const std::string fixture = "build/audio-tests/shot.wav";
        std::filesystem::copy_file("data/audio/sfx/shot_0.wav", fixture,
                                   std::filesystem::copy_options::overwrite_existing);
        const auto decoded = resources->loadAsync({{AssetKind::Audio, fixture}});
        require(waitFor(*resources, decoded, false).phase == LoadPhase::AwaitingUpload,
                "Audio CPU preparation failed");
        std::filesystem::remove(fixture);
        require(waitFor(*resources, decoded).phase == LoadPhase::Ready,
                "Audio publication reopened its deleted source");
        resources->setCacheOnly(true);
        const auto decodedId = resources->loadAudio(fixture);
        auto pcm = resources->getAudio(decodedId);
        require(pcm && !pcm->samples().empty() && pcm->sampleRate() == 48000,
                "Short effect was not decoded on the worker");

        SoundCue shot;
        shot.clips = {decodedId};
        shot.gain = .4f;
        audio->defineCue("test_shot", shot);
        const auto voice = audio->play("test_shot");
        std::vector<float> mix(4096 * 2);
        require(voice && audio->render(mix.data(), 4096) && energy(mix) > 0.000001,
                "Decoded sound produced silence");
        for (int i = 0; i < 40; ++i)
        {
            audio->render(mix.data(), 4096);
            audio->update(.085f);
        }
        require(!audio->playing(voice), "Finished one-shot voice was not retired");

        const std::string tonePath = "build/audio-tests/pitch.wav";
        writeTone(tonePath);
        AssetRequest toneRequest{AssetKind::Audio, tonePath};
        toneRequest.channels = 1;
        require(waitFor(*resources, resources->loadAsync({toneRequest})).phase == LoadPhase::Ready,
                "Tone preparation failed");
        SoundCue tone;
        tone.clips = {resources->loadAudio(tonePath, false, 1)};
        tone.loop = true;
        tone.gain = .2f;
        audio->defineCue("pitch_test", tone);
        std::vector<float> toneMix(4800 * 2);
        auto toneVoice = audio->play("pitch_test");
        audio->render(toneMix.data(), 4800);
        const auto normalCrossings = crossings(toneMix);
        audio->stopAll();
        toneVoice = audio->play("pitch_test");
        audio->setPitch(toneVoice, 2);
        audio->render(toneMix.data(), 4800);
        const auto highCrossings = crossings(toneMix);
        audio->stopAll();
        require(normalCrossings > 30 && highCrossings > normalCrossings * 1.8f &&
                    highCrossings < normalCrossings * 2.2f,
                "Backend pitch modulation did not change the rendered frequency");

        const auto bank = resources->loadAsync({{AssetKind::AudioBank, "data/audio/cues.json"}});
        auto status = waitFor(*resources, bank);
        require(status.phase == LoadPhase::Ready, status.error.c_str());
        const auto music =
            resources->getAudio(resources->loadAudio("data/audio/music/outer_space.flac", true, 2));
        require(music && music->streaming() && music->samples().empty() &&
                    music->bufferedFrames() >= 48000,
                "Streaming audio was ready before its initial PCM buffers");

        const auto engine = resources->loadAudio("data/audio/sfx/ufo_engine.wav", false, 1);
        SoundCue emitter;
        emitter.clips = {engine};
        emitter.loop = true;
        emitter.spatial = true;
        emitter.gain = .2f;
        emitter.minDistance = 10;
        emitter.maxDistance = 1000;
        emitter.maxInstances = 1;
        audio->defineCue("test_engine", emitter);
        audio->stopAll();
        audio->setListener({0, 0, 0}, {0, 0, -1}, {0, 1, 0});
        const auto hum = audio->play("test_engine", {0, 0, -10});
        require(hum && !audio->play("test_engine"),
                "Cue instance limit did not prevent duplicate loops");
        audio->render(mix.data(), 4096);
        audio->render(mix.data(), 4096);
        const double near = energy(mix);
        audio->setPosition(hum, {0, 0, -500});
        audio->render(mix.data(), 4096);
        audio->render(mix.data(), 4096);
        require(near > energy(mix) * 20, "Approaching emitter did not become louder with distance");
        audio->setPosition(hum, {-10, 0, -1});
        audio->render(mix.data(), 4096);
        audio->render(mix.data(), 4096);
        require(energy(mix, 0) > energy(mix, 1) * 1.2, "Emitter on the left did not pan left");
        audio->setPosition(hum, {10, 0, -1});
        audio->render(mix.data(), 4096);
        audio->render(mix.data(), 4096);
        require(energy(mix, 1) > energy(mix, 0) * 1.2, "Emitter on the right did not pan right");
        audio->setBusGain(AudioBus::Effects, 0);
        audio->render(mix.data(), 4096);
        audio->render(mix.data(), 4096);
        require(energy(mix) < 0.00000001, "Muted audio bus was audible");
        audio->stopAll();
        audio->setPosition(hum, {});
        require(!audio->playing(hum), "Stale voice handle remained live");

        GameAudio game(*audio);
        game.configure(*resources);
        game.enterState("title");
        for (int i = 0; i < 60; ++i)
        {
            audio->render(mix.data(), 1024);
            game.update(1024.f / 48000);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(energy(mix) > 0.000001, "Title music produced no output");
        game.enterState("game");
        game.emit({GameAudioEventType::Shot, {0, 0, -2}});
        game.emit({GameAudioEventType::Hit, {0, 0, -3}});
        game.emit({GameAudioEventType::Destroyed, {0, 0, -4}});
        for (int i = 0; i < 60; ++i)
        {
            audio->render(mix.data(), 1024);
            game.update(1024.f / 48000);
            for (float sample : mix)
                require(std::isfinite(sample) && std::abs(sample) <= 1.f,
                        "Mixer output clipped or became non-finite");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        game.enterState("win");
        game.enterState("lose");
        audio->stopAll();
        CCollidable collider(0, CAABBox());
        CGameObject ship;
        ship.setCollidable(&collider);
        CHealthController health(100, &game);
        health.attach(&ship);
        collider.receiveDamage(25);
        health.update(.016f);
        game.update(.016f);
        const unsigned afterHit = audio->activeVoices();
        require(afterHit == 1, "Damage did not emit exactly one hit cue");
        health.update(.016f);
        game.update(.016f);
        require(audio->activeVoices() == afterHit, "The same damage emitted repeated hit cues");
        collider.receiveDamage(100);
        health.update(.016f);
        game.update(.016f);
        require(ship.isDead() && audio->activeVoices() == afterHit + 1,
                "Lethal damage did not emit one destruction cue");
        collider.receiveDamage(100);
        health.update(.016f);
        game.update(.016f);
        require(audio->activeVoices() == afterHit + 1,
                "A dead ship emitted another destruction cue");
        audio->stopAll();
        {
            CGameObject emitter;
            emitter.addController(std::make_shared<CAudioEmitterController>(&game, "ufo_engine"));
            emitter.update(.016f);
            require(audio->activeVoices() == 1, "Attached emitter did not start");
        }
        audio->render(mix.data(), 4096);
        game.update(.2f);
        require(audio->activeVoices() == 0, "Destroyed object leaked its engine loop");
        auto silent = createAudioSystem(AudioOutput::Silent);
        require(!silent->audibleDevice() && silent->play("unused") == 0,
                "Silent fallback did not remain silent");
        std::cout << "Audio: async decode/stream priming, deleted-source publication, offline "
                     "mixing, attenuation, stereo pan, voice limits, buses, state music and silent "
                     "fallback passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Audio regression: " << error.what() << '\n';
        return 1;
    }
}
