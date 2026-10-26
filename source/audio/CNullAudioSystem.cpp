#include "CNullAudioSystem.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

CNullAudioSystem::~CNullAudioSystem()
{
    if (manager)
        manager->removeResourceListener(this);
}

void CNullAudioSystem::onAttach(IResourceManager *m)
{
    manager = m;
}

void CNullAudioSystem::onDetach(IResourceManager *m)
{
    if (manager == m)
        manager = nullptr;
}

void CNullAudioSystem::notify(EResourceType, ResourceId, EListenerEvent, IResourceManager *)
{
}

void CNullAudioSystem::defineCue(const std::string &, const SoundCue &)
{
}

VoiceHandle CNullAudioSystem::play(const std::string &, glm::vec3, float)
{
    return 0;
}

bool CNullAudioSystem::playing(VoiceHandle) const
{
    return false;
}

void CNullAudioSystem::stop(VoiceHandle, float)
{
}

void CNullAudioSystem::stopAll()
{
}

void CNullAudioSystem::setPosition(VoiceHandle, glm::vec3)
{
}

void CNullAudioSystem::setGain(VoiceHandle, float)
{
}

void CNullAudioSystem::setListener(glm::vec3, glm::vec3, glm::vec3)
{
}

void CNullAudioSystem::setMasterGain(float)
{
}

void CNullAudioSystem::setBusGain(AudioBus, float)
{
}

void CNullAudioSystem::update(float)
{
}

unsigned CNullAudioSystem::activeVoices() const
{
    return 0;
}

bool CNullAudioSystem::audibleDevice() const
{
    return false;
}

bool CNullAudioSystem::render(float *p, std::uint64_t n)
{
    std::fill(p, p + n * 2, 0.f);
    return true;
}
