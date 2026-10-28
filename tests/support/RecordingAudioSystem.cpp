#include "RecordingAudioSystem.h"
void RecordingAudioSystem::onAttach(IResourceManager *)
{
}
void RecordingAudioSystem::onDetach(IResourceManager *)
{
}
void RecordingAudioSystem::notify(EResourceType, ResourceId, EListenerEvent, IResourceManager *)
{
}
void RecordingAudioSystem::defineCue(const std::string &, const SoundCue &)
{
}
VoiceHandle RecordingAudioSystem::play(const std::string &cue, glm::vec3 p, float)
{
    alive = true;
    lastCue = cue;
    position = p;
    ++starts;
    return 1;
}
bool RecordingAudioSystem::playing(VoiceHandle id) const
{
    return id == 1 && alive;
}
void RecordingAudioSystem::stop(VoiceHandle id, float)
{
    if (id && alive)
        ++stops;
    alive = false;
}
void RecordingAudioSystem::stopAll()
{
    alive = false;
}
void RecordingAudioSystem::setPosition(VoiceHandle, glm::vec3 p)
{
    position = p;
}
void RecordingAudioSystem::setGain(VoiceHandle, float value)
{
    gain = value;
}
void RecordingAudioSystem::setPitch(VoiceHandle, float value)
{
    pitch = value;
}
void RecordingAudioSystem::setListener(glm::vec3, glm::vec3, glm::vec3)
{
}
void RecordingAudioSystem::setMasterGain(float)
{
}
void RecordingAudioSystem::setBusGain(AudioBus, float)
{
}
void RecordingAudioSystem::update(float)
{
}
unsigned RecordingAudioSystem::activeVoices() const
{
    return alive ? 1 : 0;
}
bool RecordingAudioSystem::audibleDevice() const
{
    return false;
}
bool RecordingAudioSystem::render(float *, std::uint64_t)
{
    return false;
}
