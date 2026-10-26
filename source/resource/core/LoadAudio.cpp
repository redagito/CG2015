#include "CResourceManager.h"
#include "audio/AudioAsset.h"
#include "resource/IResourceListener.h"

ResourceId CResourceManager::loadAudio(const std::string &file, bool stream, unsigned channels)
{
    ensureOwner();
    const auto key = file + "#" + (stream ? "stream" : "decode") + "#" + std::to_string(channels);
    if (auto found = m_audioFiles.find(key); found != m_audioFiles.end())
        return found->second;
    requireCacheHit(file);
    auto data = AudioAsset::load(file, stream, channels);
    const auto id = m_nextAudioId++;
    m_audio.emplace(id, std::move(data));
    m_audioFiles.emplace(key, id);
    notifyResourceListeners(EResourceType::Audio, id, EListenerEvent::Create);
    return id;
}
std::shared_ptr<const AudioAsset> CResourceManager::getAudio(ResourceId id) const
{
    ensureOwner();
    const auto found = m_audio.find(id);
    return found == m_audio.end() ? nullptr : found->second;
}
