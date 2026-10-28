#include "AsyncResourceState.h"
#include "CResourceManager.h"
#include "io/JsonUtil.h"
#include "resource/IResourceListener.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace
{

AssetKind assetKind(const std::string &name)
{
    if (name == "mesh")
        return AssetKind::Mesh;
    if (name == "image")
        return AssetKind::Image;
    if (name == "material")
        return AssetKind::Material;
    if (name == "model")
        return AssetKind::Model;
    if (name == "shader")
        return AssetKind::Shader;
    if (name == "text")
        return AssetKind::Text;
    if (name == "json")
        return AssetKind::Json;
    if (name == "scene")
        return AssetKind::Scene;
    if (name == "audio")
        return AssetKind::Audio;
    if (name == "enemy_roster")
        return AssetKind::EnemyRoster;
    if (name == "audio_bank")
        return AssetKind::AudioBank;
    throw std::runtime_error("Unknown asset kind: " + name);
}
} // namespace

void CResourceManager::ensureOwner() const
{
    if (std::this_thread::get_id() != m_owner)
        throw std::logic_error("Resource manager accessed from a non-owner thread");
}
void CResourceManager::requireCacheHit(const std::string &file) const
{
    if (m_cacheOnly)
        throw std::runtime_error("Asset was not prepared asynchronously: " + file);
}
bool CResourceManager::loadJsonDocument(const std::string &file, Json::Value &output)
{
    ensureOwner();
    if (const auto found = m_documents.find(file); found != m_documents.end())
    {
        output = found->second;
        return true;
    }
    requireCacheHit(file);
    Json::Value root;
    if (!::load(file, root))
        return false;
    m_documents[file] = root;
    output = std::move(root);
    return true;
}
AsyncLoadId CResourceManager::loadAsync(const std::vector<AssetRequest> &requests)
{
    return beginAsync(requests, {});
}
AsyncLoadId CResourceManager::loadManifestAsync(const std::string &file)
{
    return beginAsync({}, file);
}
AsyncLoadId CResourceManager::beginAsync(std::vector<AssetRequest> requests, std::string manifest)
{
    ensureOwner();
    if (!m_async)
        m_async = std::make_unique<AsyncState>();
    // Bounded concurrency. A batch resolves its own dependent materials/models.
    unsigned running = 0;
    for (const auto &[id, batch] : m_async->batches)
        if (batch->phase.load() == LoadPhase::Preparing)
            ++running;
    if (running >= 2)
        throw std::runtime_error(
            "Two asset batches are already preparing; wait or cancel before queuing another");
    const auto id = m_async->next++;
    auto batch = std::make_shared<AsyncBatch>();
    m_async->batches.emplace(id, batch);
    batch->worker = std::jthread(
        [batch, requests = std::move(requests),
         manifest = std::move(manifest)](std::stop_token stop) mutable
        {
            try
            {
                auto staged = std::make_unique<CResourceManager>();
                staged->m_onPrepared = [stop]
                {
                    if (stop.stop_requested())
                        throw Cancelled{};
                };
                if (!manifest.empty())
                {
                    Json::Value root;
                    if (!staged->loadJsonDocument(manifest, root) || !root["assets"].isArray())
                        throw std::runtime_error("Invalid asset manifest: " + manifest);
                    for (const auto &node : root["assets"])
                    {
                        const auto format = node.get("format", "rgb").asString();
                        if (format != "rgb" && format != "rgba" && format != "gray")
                            throw std::runtime_error("Unknown image format in " + manifest);
                        requests.push_back(
                            {assetKind(node["kind"].asString()), node["file"].asString(),
                             format == "rgba"   ? EColorFormat::RGBA32
                             : format == "gray" ? EColorFormat::GreyScale8
                                                : EColorFormat::RGB24,
                             node.get("stream", false).asBool(), node.get("channels", 2).asUInt()});
                    }
                }
                batch->total = static_cast<unsigned>(requests.size());
                for (const auto &request : requests)
                {
                    if (stop.stop_requested())
                        throw Cancelled{};
                    ResourceId resource = 0;
                    switch (request.kind)
                    {
                    case AssetKind::Mesh:
                        resource = staged->loadMesh(request.file);
                        break;
                    case AssetKind::Image:
                        resource = staged->loadImage(request.file, request.format);
                        break;
                    case AssetKind::Material:
                        resource = staged->loadMaterial(request.file);
                        break;
                    case AssetKind::Model:
                        resource = staged->loadModel(request.file);
                        break;
                    case AssetKind::Shader:
                        resource = staged->loadShader(request.file);
                        break;
                    case AssetKind::Text:
                        resource = staged->loadString(request.file);
                        break;
                    case AssetKind::Audio:
                        resource =
                            staged->loadAudio(request.file, request.stream, request.channels);
                        break;
                    case AssetKind::AudioBank:
                    {
                        Json::Value bank;
                        if (!staged->loadJsonDocument(request.file, bank) ||
                            !bank["cues"].isObject())
                            throw std::runtime_error("Invalid audio bank: " + request.file);
                        for (const auto &cue : bank["cues"])
                        {
                            if (!cue["files"].isArray() || cue["files"].empty())
                                throw std::runtime_error("Audio cue has no variations");
                            for (const auto &path : cue["files"])
                                staged->loadAudio(path.asString(),
                                                  cue.get("stream", false).asBool(),
                                                  cue.get("spatial", false).asBool() ? 1u : 2u);
                        }
                        break;
                    }
                    case AssetKind::EnemyRoster:
                    case AssetKind::Json:
                    case AssetKind::Scene:
                    {
                        Json::Value root;
                        if (!staged->loadJsonDocument(request.file, root))
                            throw std::runtime_error("Cannot parse " + request.file);
                        if (request.kind == AssetKind::EnemyRoster)
                        {
                            if (!root["enemy_types"].isArray())
                                throw std::runtime_error("Enemy roster needs enemy_types");
                            const auto prepare = [&](const Json::Value &type)
                            {
                                if (!type["mesh"].isString() || !type["material"].isString())
                                    throw std::runtime_error(
                                        "Enemy assets need mesh and material paths");
                                if (staged->loadMesh(type["mesh"].asString()) == invalidResource ||
                                    staged->loadMaterial(type["material"].asString()) ==
                                        invalidResource)
                                    throw std::runtime_error("Cannot prepare enemy assets for " +
                                                             request.file);
                            };
                            for (const auto &type : root["enemy_types"])
                            {
                                prepare(type);
                                if (type.isMember("decoration"))
                                    prepare(type["decoration"]);
                            }
                        }
                        if (request.kind == AssetKind::Scene)
                            for (const auto &object : root["scene_objects"])
                            {
                                for (const auto &[name, kind] : {std::pair{"mesh", AssetKind::Mesh},
                                                                 {"material", AssetKind::Material},
                                                                 {"model", AssetKind::Model}})
                                {
                                    if (!object.isMember(name))
                                        continue;
                                    const auto path = object[name].asString();
                                    ResourceId dependency =
                                        kind == AssetKind::Mesh       ? staged->loadMesh(path)
                                        : kind == AssetKind::Material ? staged->loadMaterial(path)
                                                                      : staged->loadModel(path);
                                    if (dependency == invalidResource)
                                        throw std::runtime_error("Cannot prepare " + path +
                                                                 " for " + request.file);
                                }
                            }
                        break;
                    }
                    }
                    if (resource == invalidResource)
                        throw std::runtime_error("Cannot prepare asset: " + request.file);
                    ++batch->prepared;
                }
                if (stop.stop_requested())
                    throw Cancelled{};
                auto append = [&](EResourceType type, const auto &files)
                {
                    std::vector<std::pair<ResourceId, std::string>> ordered;
                    for (const auto &[file, local] : files)
                        ordered.emplace_back(local, file);
                    std::sort(ordered.begin(), ordered.end());
                    for (const auto &[local, file] : ordered)
                        batch->publications.push_back({type, local, file});
                };
                append(EResourceType::String, staged->m_textFiles);
                append(EResourceType::Mesh, staged->m_meshFiles);
                append(EResourceType::Image, staged->m_imageFiles);
                append(EResourceType::Audio, staged->m_audioFiles);
                append(EResourceType::Material, staged->m_materialFiles);
                append(EResourceType::Model, staged->m_modelFiles);
                append(EResourceType::Shader, staged->m_shaderFiles);
                for (const auto &[file, root] : staged->m_documents)
                    batch->publications.push_back(
                        {EResourceType::String, invalidResource, file, true});
                staged->m_onPrepared = {};
                batch->staged = std::move(staged);
                // The owner may access staged storage only after this release.
                batch->phase.store(LoadPhase::AwaitingUpload, std::memory_order_release);
            }
            catch (const Cancelled &)
            {
                batch->phase.store(LoadPhase::Cancelled, std::memory_order_release);
            }
            catch (const std::exception &error)
            {
                batch->error = error.what();
                batch->phase.store(LoadPhase::Failed, std::memory_order_release);
            }
        });
    return id;
}
AsyncLoadStatus CResourceManager::getLoadStatus(AsyncLoadId id) const
{
    ensureOwner();
    if (!m_async || !m_async->batches.contains(id))
        return {LoadPhase::Failed, 0, 0, 0, 0, "Unknown loading request"};
    const auto &batch = *m_async->batches.at(id);
    const auto phase = batch.phase.load(std::memory_order_acquire);
    AsyncLoadStatus status{phase, batch.prepared.load(), batch.total.load(), batch.published};
    if (phase != LoadPhase::Preparing)
        status.uploadTotal = static_cast<unsigned>(batch.publications.size());
    if (phase == LoadPhase::Failed)
        status.error = batch.error;
    return status;
}
void CResourceManager::cancelLoad(AsyncLoadId id)
{
    ensureOwner();
    if (!m_async || !m_async->batches.contains(id))
        return;
    auto &batch = *m_async->batches.at(id);
    batch.worker.request_stop();
    const auto phase = batch.phase.load(std::memory_order_acquire);
    if (phase == LoadPhase::AwaitingUpload || phase == LoadPhase::Uploading)
        batch.phase = LoadPhase::Cancelled;
}
void CResourceManager::stopAsync()
{
    if (!m_async)
        return;
    for (auto &[id, batch] : m_async->batches)
        batch->worker.request_stop();
    for (auto &[id, batch] : m_async->batches)
        if (batch->worker.joinable())
            batch->worker.join();
    m_async.reset();
}
void CResourceManager::pumpAsyncLoads(double budgetMilliseconds)
{
    ensureOwner();
    if (!m_async || !std::isfinite(budgetMilliseconds) || budgetMilliseconds <= 0)
        return;
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::duration<double, std::milli>(budgetMilliseconds);
    for (auto &[id, shared] : m_async->batches)
    {
        auto &batch = *shared;
        auto phase = batch.phase.load(std::memory_order_acquire);
        if (phase != LoadPhase::AwaitingUpload && phase != LoadPhase::Uploading)
            continue;
        if (batch.worker.get_stop_token().stop_requested())
        {
            batch.phase = LoadPhase::Cancelled;
            continue;
        }
        batch.phase = LoadPhase::Uploading;
        try
        {
            do
            {
                if (batch.published == batch.publications.size())
                {
                    batch.staged.reset();
                    batch.phase = LoadPhase::Ready;
                    break;
                }
                const auto &item = batch.publications[batch.published];
                auto &staged = *batch.staged;
                if (item.document)
                {
                    m_documents.try_emplace(item.key, std::move(staged.m_documents.at(item.key)));
                    ++batch.published;
                    continue;
                }
                auto map = [&](EResourceType type, ResourceId local)
                {
                    return local == invalidResource
                               ? invalidResource
                               : batch.remap[static_cast<size_t>(type)].at(local);
                };
                auto publish = [&](auto &source, auto &destination, auto &files, ResourceId &next)
                {
                    ResourceId result;
                    if (auto cached = files.find(item.key); cached != files.end())
                        result = cached->second;
                    else
                    {
                        result = next++;
                        destination.emplace(result, std::move(source.at(item.local)));
                        files.emplace(item.key, result);
                        try
                        {
                            notifyResourceListeners(item.type, result, EListenerEvent::Create);
                        }
                        catch (...)
                        {
                            // A failed GPU upload must not leave a false cache hit
                            // that would make a later retry appear successful.
                            try
                            {
                                notifyResourceListeners(item.type, result, EListenerEvent::Delete);
                            }
                            catch (...)
                            {
                            }
                            files.erase(item.key);
                            destination.erase(result);
                            throw;
                        }
                    }
                    batch.remap[static_cast<size_t>(item.type)][item.local] = result;
                };
                switch (item.type)
                {
                case EResourceType::String:
                    publish(staged.m_strings, m_strings, m_textFiles, m_nextStringId);
                    break;
                case EResourceType::Mesh:
                    publish(staged.m_meshes, m_meshes, m_meshFiles, m_nextMeshId);
                    break;
                case EResourceType::Image:
                    publish(staged.m_images, m_images, m_imageFiles, m_nextImageId);
                    break;
                case EResourceType::Audio:
                    publish(staged.m_audio, m_audio, m_audioFiles, m_nextAudioId);
                    break;
                case EResourceType::Material:
                {
                    auto &m = staged.m_materials.at(item.local);
                    for (auto *dep : {&m.m_base, &m.m_normal, &m.m_specular, &m.m_glow, &m.m_alpha})
                        *dep = map(EResourceType::Image, *dep);
                    publish(staged.m_materials, m_materials, m_materialFiles, m_nextMaterialId);
                    break;
                }
                case EResourceType::Model:
                {
                    auto &m = staged.m_models.at(item.local);
                    m.m_mesh = map(EResourceType::Mesh, m.m_mesh);
                    m.m_material = map(EResourceType::Material, m.m_material);
                    publish(staged.m_models, m_models, m_modelFiles, m_nextModelId);
                    break;
                }
                case EResourceType::Shader:
                {
                    auto &s = staged.m_shaders.at(item.local);
                    for (auto *dep :
                         {&s.m_vertex, &s.m_tessCtrl, &s.m_tessEval, &s.m_geometry, &s.m_fragment})
                        *dep = map(EResourceType::String, *dep);
                    publish(staged.m_shaders, m_shaders, m_shaderFiles, m_nextShaderId);
                    break;
                }
                }
                ++batch.published;
            } while (std::chrono::steady_clock::now() < deadline);
        }
        catch (const std::exception &error)
        {
            batch.error = error.what();
            batch.phase = LoadPhase::Failed;
        }
        if (std::chrono::steady_clock::now() >= deadline)
            break;
    }
}
