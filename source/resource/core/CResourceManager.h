#pragma once

#include <json/json.h>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "resource/IResourceManager.h"

#include "SImage.h"
#include "SMaterial.h"
#include "SMesh.h"
#include "SModel.h"
#include "SShader.h"

/**
 * \brief Resource manager implementation.
 */
class CResourceManager : public IResourceManager
{
  public:
    ResourceId loadAudio(const std::string &file, bool stream = false,
                         unsigned channels = 2) override;
    std::shared_ptr<const AudioAsset> getAudio(ResourceId id) const override;
    AsyncLoadId loadAsync(const std::vector<AssetRequest> &requests) override;
    AsyncLoadId loadManifestAsync(const std::string &file) override;
    AsyncLoadStatus getLoadStatus(AsyncLoadId id) const override;
    void cancelLoad(AsyncLoadId id) override;
    void pumpAsyncLoads(double budgetMilliseconds = 2.0) override;
    bool loadJsonDocument(const std::string &file, Json::Value &output) override;
    void setCacheOnly(bool enabled) override
    {
        ensureOwner();
        m_cacheOnly = enabled;
    }
    SMaterialSettings getMaterialSettings(ResourceId id) const override;
    bool setMaterialSettings(ResourceId id, const SMaterialSettings &settings) override;
    bool updateString(ResourceId id, const std::string &text) override;
    bool reloadShader(ResourceId id) override;
    bool reloadResource(EResourceType type, ResourceId id) override;
    bool removeResource(EResourceType type, ResourceId id) override;
    CResourceManager();
    ~CResourceManager();

    ResourceId createMesh(const std::vector<float> &vertices,
                          const std::vector<unsigned int> &indices,
                          const std::vector<float> &normals, const std::vector<float> &uvs,
                          EPrimitiveType type);

    ResourceId loadMesh(const std::string &file);

    bool getMesh(ResourceId id, std::vector<float> &vertices, std::vector<unsigned int> &indices,
                 std::vector<float> &normals, std::vector<float> &uvs, EPrimitiveType &type) const;

    ResourceId createImage(const std::vector<unsigned char> &imageData, unsigned int width,
                           unsigned int height, EColorFormat format);

    ResourceId loadImage(const std::string &file, EColorFormat format);

    bool getImage(ResourceId id, std::vector<unsigned char> &data, unsigned int &width,
                  unsigned int &height, EColorFormat &format) const;

    ResourceId createMaterial(ResourceId base, ResourceId normal, ResourceId specular,
                              ResourceId glow, ResourceId alpha);

    ResourceId loadMaterial(const std::string &file);

    bool getMaterial(ResourceId id, ResourceId &base, ResourceId &normal, ResourceId &specular,
                     ResourceId &glow, ResourceId &alpha) const;

    ResourceId createModel(ResourceId mesh, ResourceId material);

    ResourceId loadModel(const std::string &file);

    bool getModel(ResourceId id, ResourceId &mesh, ResourceId &material);

    ResourceId createString(const std::string &text);

    ResourceId loadString(const std::string &file);
    ResourceId loadString(const std::string &file, bool doPreprocessing);

    bool getString(ResourceId id, std::string &text) const;

    ResourceId createShader(ResourceId vertex, ResourceId tessCtrl, ResourceId tessEval,
                            ResourceId geometry, ResourceId fragment);

    ResourceId loadShader(const std::string &file);

    bool getShader(ResourceId id, ResourceId &vertex, ResourceId &tessCtrl, ResourceId &tessEval,
                   ResourceId &geometry, ResourceId &fragment) const;

    void addResourceListener(IResourceListener *listener);
    void removeResourceListener(IResourceListener *listener);

  protected:
    void notifyResourceListeners(EResourceType type, ResourceId id, EListenerEvent event);

  private:
    struct Cancelled
    {
    };
    ResourceId m_nextAudioId = 0;
    std::unordered_map<ResourceId, std::shared_ptr<AudioAsset>> m_audio;
    std::unordered_map<std::string, ResourceId> m_audioFiles;
    struct AsyncState;
    std::unique_ptr<AsyncState> m_async;
    AsyncLoadId beginAsync(std::vector<AssetRequest> requests, std::string manifest);
    void stopAsync();
    void ensureOwner() const;
    void requireCacheHit(const std::string &file) const;
    const std::thread::id m_owner = std::this_thread::get_id();
    bool m_cacheOnly = false;
    std::function<void()> m_onPrepared;
    std::unordered_map<std::string, Json::Value> m_documents;
    ResourceId m_nextMeshId = 0;     /**< Next free mesh id. */
    ResourceId m_nextImageId = 0;    /**< Next free image id. */
    ResourceId m_nextMaterialId = 0; /**< Next free material id. */
    ResourceId m_nextModelId = 0;    /**< Next free model id. */
    ResourceId m_nextStringId = 0;   /**< Next free string id. */
    ResourceId m_nextShaderId = 0;   /**< Next free shader id. */

    // TODO Change to vector?
    std::unordered_map<ResourceId, SMesh> m_meshes;        /**< Loaded meshes. */
    std::unordered_map<ResourceId, SImage> m_images;       /**< Loaded images. */
    std::unordered_map<ResourceId, SMaterial> m_materials; /**< Loaded materials. */
    std::unordered_map<ResourceId, SModel> m_models;       /**< Loaded models. */
    std::unordered_map<ResourceId, std::string> m_strings; /**< Loaded strings. */
    std::unordered_map<ResourceId, SShader> m_shaders;     /**< Loaded shaders. */

    std::unordered_map<std::string, ResourceId>
        m_meshFiles; /**< Maps mesh file to mesh resource id. */
    std::unordered_map<std::string, ResourceId>
        m_imageFiles; /**< Maps image file to image resource id. */
    std::unordered_map<std::string, ResourceId>
        m_materialFiles; /**< Maps material file to material resource id. */
    std::unordered_map<std::string, ResourceId>
        m_modelFiles; /**< Maps model file to model resource id. */
    std::unordered_map<std::string, ResourceId>
        m_textFiles; /**< Maps text file to string resource id. */
    std::unordered_map<std::string, ResourceId>
        m_shaderFiles; /**< Maps shader program file to shader resource id. */

    std::list<IResourceListener *> m_resourceListeners; /**< Registered listeners. */
};
