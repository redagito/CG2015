#include "CScene.h"

#include "CSceneQuery.h"
#include "SSceneDirectionalLight.h"
#include "SSceneObject.h"
#include "SScenePointLight.h"

#include "collision/CFrustum.h"

#include "graphics/ICamera.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CModel.h"
#include <stdexcept>

#include <spdlog/spdlog.h>

bool CScene::getViewFrustumCulling()
{
    return s_useViewFrustumCulling;
}

void CScene::setViewFrustumCulling(bool enable)
{
    s_useViewFrustumCulling = enable;
}

bool CScene::s_useViewFrustumCulling = true;

CScene::CScene(const IGraphicsResourceManager *manager) : m_resourceManager(manager)
{
}

CScene::~CScene()
{
}

SceneObjectId CScene::createObject(ResourceId model, const glm::vec3 &position,
                                   const glm::quat &rotation, const glm::vec3 &scale)
{
    const auto *resource = m_resourceManager->getModel(model);
    if (!resource || !resource->hasMesh())
        throw std::runtime_error("Invalid model handle");
    const auto id =
        createObject(resource->getMeshId(), resource->getMaterialId(), position, rotation, scale);
    m_objects.at(id).m_model = model;
    return id;
}

SceneObjectId CScene::createObject(ResourceId meshId, ResourceId material,
                                   const glm::vec3 &position, const glm::quat &rotation,
                                   const glm::vec3 &scale)
{
    const CMesh *meshPtr = m_resourceManager->getMesh(meshId);
    if (!meshPtr)
        throw std::runtime_error("Invalid mesh handle");
    if (!m_freeObjects.empty())
    {
        const auto id = m_freeObjects.back();
        m_freeObjects.pop_back();
        m_objects[id] = SSceneObject(meshId, material, position, rotation, scale, true,
                                     meshPtr->getBoundingSphere());
        return id;
    }
    m_objects.push_back(SSceneObject(meshId, material, position, rotation, scale, true,
                                     meshPtr->getBoundingSphere()));
    return m_objects.size() - 1;
}

bool CScene::getObject(SceneObjectId id, ResourceId &mesh, ResourceId &material,
                       glm::vec3 &position, glm::quat &rotation, glm::vec3 &scale,
                       bool &visible) const
{
    // TODO Needs to be changed for better data structures
    if (id < 0 || ((unsigned int)id) >= m_objects.size())
    {
        return false;
    }

    if (m_objects.at(id).m_mesh == invalidResource && m_objects.at(id).m_model == invalidResource)
        return false;
    // Write data
    mesh = m_objects.at(id).m_mesh;
    material = m_objects.at(id).m_material;
    if (m_objects.at(id).m_model != invalidResource)
    {
        const auto *model = m_resourceManager->getModel(m_objects.at(id).m_model);
        if (!model || !model->hasMesh())
            return false;
        mesh = model->getMeshId();
        material = model->getMaterialId();
    }
    position = m_objects.at(id).m_position;
    rotation = m_objects.at(id).m_rotation;
    scale = m_objects.at(id).m_scale;
    visible = m_objects.at(id).m_visible;
    return true;
}

void CScene::setObject(SceneObjectId id, ResourceId meshId, ResourceId material,
                       const glm::vec3 &position, const glm::quat &rotation, const glm::vec3 &scale,
                       bool visible)
{
    // TODO Needs to be changed for better data structures
    assert(id >= 0 && ((unsigned int)id) < m_objects.size() && "Invalid scene object id");
    unsigned int index = (unsigned int)id;
    // Write data
    const CMesh *meshPtr = m_resourceManager->getMesh(meshId);
    m_objects[index] = SSceneObject(meshId, material, position, rotation, scale, visible,
                                    meshPtr->getBoundingSphere());
    return;
}

SceneObjectId CScene::createPointLight(const glm::vec3 &position, float radius,
                                       const glm::vec3 &color, float intensity, bool castsShadow)
{
    m_pointLights.push_back(SScenePointLight(position, radius, color, intensity, castsShadow));
    return m_pointLights.size() - 1;
}

bool CScene::getPointLight(SceneObjectId id, glm::vec3 &position, float &radius, glm::vec3 &color,
                           float &intensity, bool &castsShadow) const
{
    // TODO Needs to be changed for better data structures
    if (id < 0 || ((unsigned int)id) >= m_pointLights.size())
    {
        return false;
    }

    // Write data
    position = m_pointLights.at(id).m_position;
    radius = m_pointLights.at(id).m_radius;
    color = m_pointLights.at(id).m_color;
    intensity = m_pointLights.at(id).m_intensity;
    castsShadow = m_pointLights.at(id).m_castsShadow;
    return true;
}

void CScene::setPointLight(SceneObjectId id, const glm::vec3 &position, float radius,
                           const glm::vec3 &color, float intensity, bool castsShadow)
{
    // TODO Needs to be changed for better data structures
    assert(id >= 0 && ((unsigned int)id) < m_pointLights.size() && "Invalid scene object id");

    // Write data
    m_pointLights[id].m_position = position;
    m_pointLights[id].m_radius = radius;
    m_pointLights[id].m_color = color;
    m_pointLights[id].m_intensity = intensity;
    m_pointLights[id].m_castsShadow = castsShadow;
    return;
}

SceneObjectId CScene::createDirectionalLight(const glm::vec3 &direction, const glm::vec3 &color,
                                             float intensity, bool castsShadow)
{
    m_directionalLights.push_back(SSceneDirectionalLight(direction, color, intensity, castsShadow));
    return m_directionalLights.size() - 1;
}

bool CScene::getDirectionalLight(SceneObjectId id, glm::vec3 &direction, glm::vec3 &color,
                                 float &intensity, bool &castsShadow) const
{
    // TODO Needs to be changed for better data structures
    if (id < 0 || ((unsigned int)id) >= m_directionalLights.size())
    {
        return false;
    }

    // Write data
    direction = m_directionalLights.at(id).m_direction;
    color = m_directionalLights.at(id).m_color;
    intensity = m_directionalLights.at(id).m_intensity;
    castsShadow = m_directionalLights.at(id).m_castsShadow;
    return true;
}

void CScene::setDirectionalLight(SceneObjectId id, const glm::vec3 &direction,
                                 const glm::vec3 &color, float intensity, bool castsShadow)
{
    // TODO Needs to be changed for better data structures
    assert(id >= 0 && ((unsigned int)id) < m_directionalLights.size() && "Invalid scene object id");

    // Write data
    m_directionalLights[id].m_direction = direction;
    m_directionalLights[id].m_color = color;
    m_directionalLights[id].m_intensity = intensity;
    m_directionalLights[id].m_castsShadow = castsShadow;
    return;
}

void CScene::setAmbientLight(const glm::vec3 &color, float intensity)
{
    m_ambientColor = color;
    m_ambientIntensity = intensity;
}

bool CScene::getAmbientLight(glm::vec3 &color, float &intensity) const
{
    color = m_ambientColor;
    intensity = m_ambientIntensity;
    return true;
}

void CScene::getVisibleObjects(const ICamera &camera, ISceneQuery &query) const
{
    // Create frustum from camera matrices
    CFrustum viewFrustum;
    viewFrustum.setFromViewProjectionClipSpaceApproach(camera.getView(), camera.getProjection());

    // Hackyyy
    camera.getFeatureInfo().culledObjectCount = 0;

    // TODO Occlusion culling, better data structure for objects
    for (unsigned int i = 0; i < m_objects.size(); ++i)
    {
        // Return only objects with visibility flag set
        if (m_objects.at(i).m_visible)
        {
            // Check the objects bounding sphere against the view frustum or do not cull if disabled
            ResourceId meshId, materialId;
            glm::vec3 position, scale;
            glm::quat rotation;
            bool visible;
            if (!getObject(i, meshId, materialId, position, rotation, scale, visible))
                continue;
            const auto *mesh = m_resourceManager->getMesh(meshId);
            if (!mesh)
                continue;
            const SSceneObject current(meshId, materialId, position, rotation, scale, visible,
                                       mesh->getBoundingSphere());
            if (!s_useViewFrustumCulling || !camera.getFeatureInfo().frustumCullingActive ||
                viewFrustum.isInsideOrIntersects(current.boundingSphere))
            {
                // Object is (at least partially) visible, add to query result
                // Counter variable is object id
                query.addObject(i);
            }
            else
            {
                camera.getFeatureInfo().culledObjectCount += 1;
            }
        }
    }

    // Add visible point Lights
    for (unsigned int i = 0; i < m_pointLights.size(); ++i)
    {
        // Check light volume against view frustum for point light culling
        if (!s_useViewFrustumCulling || !camera.getFeatureInfo().frustumCullingActive ||
            viewFrustum.isInsideOrIntersects(
                CBoundingSphere(m_pointLights.at(i).m_position, m_pointLights.at(i).m_radius)))
        {
            // Counter variable is light id
            query.addPointLight(i);
        }
    }

    // TODO Directional light culling?
    // For now add all directional lights
    for (unsigned int i = 0; i < m_directionalLights.size(); ++i)
    {
        // Counter variable is light id
        query.addDirectionalLight(i);
    }
    return;
}

bool CScene::removeObject(SceneObjectId id)
{
    if (id < 0 || static_cast<size_t>(id) >= m_objects.size())
        return false;
    if (m_objects[id].m_mesh == invalidResource && m_objects[id].m_model == invalidResource)
        return false;
    m_freeObjects.push_back(id);
    m_objects[id] = SSceneObject{};
    m_objects[id].m_visible = false;
    return true;
}

void CScene::setParticles(const std::vector<RenderParticle> &particles)
{
    m_particles = particles;
}
const std::vector<RenderParticle> &CScene::particles() const
{
    return m_particles;
}

void CScene::reset()
{
    m_objects.clear();
    m_freeObjects.clear();
    m_pointLights.clear();
    m_directionalLights.clear();
    m_particles.clear();
    m_ambientColor = glm::vec3(1);
    m_ambientIntensity = 0;
}
