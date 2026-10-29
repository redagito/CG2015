#include "GeometryPass.h"
#include "MaterialDraw.h"

#include <algorithm>

#include <cassert>
#include <string>

#include <glm/ext.hpp>

#include "graphics/ICamera.h"
#include "graphics/IScene.h"
#include "graphics/IWindow.h"
#include "graphics/scene/CSceneQuery.h"

#include "graphics/camera/CStaticCamera.h"
#include "resource/IResourceManager.h"

#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CMaterial.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CShaderProgram.h"
#include "graphics/resource/CTexture.h"

#include "graphics/renderer/Draw.h"
#include "graphics/renderer/core/RendererCoreConfig.h"

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>

void GeometryPass::render(const IScene &scene, const ICamera &camera, const IWindow &window,
                          const IGraphicsResourceManager &manager, ISceneQuery &query)
{
    m_frame.hasBlendedObjects = false;
    // Set framebuffer
    m_gbuffer.framebuffer.setActive(GL_FRAMEBUFFER);
    m_gbuffer.framebuffer.resize(window.getWidth(), window.getHeight());
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    // Clear buffer
    // TODO Should be retrieved as scene parameters
    // Diffuse, glow
    GLfloat diffuseGlow[] = {0.f, 0.f, 0.f, 0.f};
    GLfloat normalSpecular[] = {0.5f, 0.5f, 1.f, 0.f};
    glClearBufferfv(GL_COLOR, 0, diffuseGlow);
    // Normal, specular
    glClearBufferfv(GL_COLOR, 1, normalSpecular);

    // Clear
    glClear(GL_DEPTH_BUFFER_BIT);

    // Geometry pass, uses gbuffer fbo
    CShaderProgram *geometryPassShader = manager.getShaderProgram(m_geometryPassShaderId);
    if (geometryPassShader == nullptr)
    {
        SPDLOG_ERROR("Failed to retrieve geometry pass shader.");
        return;
    }
    geometryPassShader->setUniform("normal_mapping_enabled",
                                   static_cast<int>(camera.getFeatureInfo().normalMappingActive));

    // Depth
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // Backface culling disabled for debugging
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Winding order, standard is counter-clockwise
    glFrontFace(GL_CCW);

    // Reset viewport
    glViewport(0, 0, window.getWidth(), window.getHeight());

    // Set view and projection matrices
    m_frame.transform.setViewMatrix(camera.getView());
    m_frame.transform.setProjectionMatrix(camera.getProjection());

    // Send view/projection to default shader
    geometryPassShader->setUniform(viewMatrixUniformName, m_frame.transform.getViewMatrix());
    geometryPassShader->setUniform(projectionMatrixUniformName,
                                   m_frame.transform.getProjectionMatrix());

    m_instances.begin();
    // Traverse visible objects
    while (query.hasNextObject())
    {
        // Get next visible object
        SceneObjectId id = query.getNextObject();

        // Object attributes
        ResourceId meshId = -1;
        ResourceId materialId = -1;
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        bool visible;

        // Retrieve object data
        if (!scene.getObject(id, meshId, materialId, position, rotation, scale, visible))
        {
            // Invalid id
            SPDLOG_ERROR("Invalid scene object id {}.", id);
        }
        else
        {
            // Resolve ids
            CMaterial *material = manager.getMaterial(materialId);
            if (material && material->settings.alphaMode == AlphaMode::Blend)
                m_frame.hasBlendedObjects = true;
            if (material &&
                (material->settings.background || material->settings.alphaMode == AlphaMode::Blend))
                continue;

            // Set transformations
            m_frame.transform.setPosition(position);
            m_frame.transform.setRotation(rotation);
            m_frame.transform.setScale(scale);

            m_instances.add(meshId, materialId,
                            m_frame.transform.getTranslationMatrix() *
                                m_frame.transform.getRotationMatrix() *
                                m_frame.transform.getScaleMatrix(),
                            scale.x * scale.y * scale.z < 0);
        }
    }

    m_instances.draw(manager, *geometryPassShader);
    // Post draw error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    // Disable geometry buffer
    m_gbuffer.framebuffer.setInactive(GL_FRAMEBUFFER);
}

bool GeometryPass::init(IResourceManager &manager)
{
    // Init geometry pass shader
    // TODO Read file name from config?
    // Geometry pass shader for filling gbuffer
    std::string geometryPassShaderFile("data/shader/deferred/geometry_pass.ini");
    m_geometryPassShaderId = manager.loadShader(geometryPassShaderFile);

    // Check if ok
    if (m_geometryPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                     geometryPassShaderFile.c_str());
        return false;
    }

    // Init gbuffer
    // Diffuse texture, stores base color and glow mask.
    m_gbuffer.albedoGlow = std::make_shared<CTexture>();
    m_gbuffer.albedoGlow->init(800, 600, GL_RGBA16F);

    // Normal texture, store x and y, z normals and specularity.
    m_gbuffer.normalSpecular = std::make_shared<CTexture>();
    m_gbuffer.normalSpecular->init(800, 600, GL_RGBA16F);

    // Depth texture
    m_gbuffer.depth = std::make_shared<CTexture>();
    m_gbuffer.depth->init(800, 600, GL_DEPTH_COMPONENT24);

    // Error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    // Framebuffer attachments
    m_gbuffer.framebuffer.attach(m_gbuffer.depth, GL_DEPTH_ATTACHMENT);
    m_gbuffer.framebuffer.attach(m_gbuffer.albedoGlow, GL_COLOR_ATTACHMENT0);
    m_gbuffer.framebuffer.attach(m_gbuffer.normalSpecular, GL_COLOR_ATTACHMENT1);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    SPDLOG_INFO("GBuffer state: {}.", m_gbuffer.framebuffer.getState().c_str());

    // Reset framebuffer
    m_gbuffer.framebuffer.setInactive(GL_FRAMEBUFFER);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }
    return true;
}
