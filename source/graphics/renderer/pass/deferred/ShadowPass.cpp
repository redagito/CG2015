#include "ShadowPass.h"
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

const ShadowPass::CameraDirection ShadowPass::g_cameraDirections[6] = {
    {GL_TEXTURE_CUBE_MAP_POSITIVE_X, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_X, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Y, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)},
    {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)},
    {GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)}};

void ShadowPass::renderDirectional(const IScene &scene, const ICamera &camera,
                                   const IWindow &window, const IGraphicsResourceManager &manager)
{
    CShaderProgram *shadowMapPassShader = manager.getShaderProgram(m_shadowMapPassShaderId);

    // Set framebuffer
    m_shadows.directionalFramebuffer.setActive(GL_FRAMEBUFFER);
    const auto resolution = std::clamp(camera.getFeatureInfo().shadowMapResolution, 64u, 4096u);
    m_shadows.directionalFramebuffer.resize(resolution, resolution);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    shadowMapPassShader->setActive();

    // Clear
    glClear(GL_DEPTH_BUFFER_BIT);

    // Depth
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // Backface culling disabled for debugging
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Winding order, standard is counter-clockwise
    glFrontFace(GL_CCW);

    // Reset viewport
    glViewport(0, 0, resolution, resolution);

    // Stores active transformations
    CTransformer transformer;

    // Set view and projection matrices
    transformer.setViewMatrix(camera.getView());
    transformer.setProjectionMatrix(camera.getProjection());

    // Query visible scene objects
    CSceneQuery query;
    scene.getVisibleObjects(camera, query);

    // Send view/projection to default shader
    shadowMapPassShader->setUniform(viewMatrixUniformName, transformer.getViewMatrix());
    shadowMapPassShader->setUniform(projectionMatrixUniformName, transformer.getProjectionMatrix());

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
            CMesh *mesh = manager.getMesh(meshId);
            CMaterial *material = manager.getMaterial(materialId);
            if (material &&
                (material->settings.background || material->settings.alphaMode == AlphaMode::Blend))
                continue;

            // Set transformations
            transformer.setPosition(position);
            transformer.setRotation(rotation);
            transformer.setScale(scale);

            // Forward draw call
            drawMaterialMesh(mesh, transformer.getTranslationMatrix(),
                             transformer.getRotationMatrix(), transformer.getScaleMatrix(),
                             material, manager, shadowMapPassShader);
        }
    }

    // Post draw error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    // Disable geometry buffer
    m_shadows.directionalFramebuffer.setInactive(GL_FRAMEBUFFER);

    glCullFace(GL_BACK);
}

void ShadowPass::renderPoint(const IScene &scene, const ICamera &camera, const IWindow &window,
                             const IGraphicsResourceManager &manager)
{
    CShaderProgram *shadowCubePassShader = manager.getShaderProgram(m_shadowCubePassShaderId);
    shadowCubePassShader->setActive();

    // Depth
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // Backface culling disabled for debugging
    glDisable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glDisable(GL_BLEND);

    // Winding order, standard is counter-clockwise
    glFrontFace(GL_CCW);

    // Reset viewport
    const auto resolution = std::clamp(camera.getFeatureInfo().pointShadowResolution, 64u, 2048u);
    m_shadows.pointDepth->resize(resolution, resolution);
    m_shadows.pointCube->resize(resolution, resolution);
    glViewport(0, 0, resolution, resolution);
    // m_shadows.directionalFramebuffer.resize(1024, 1024);

    shadowCubePassShader->setUniform(lightPositionUniformName, camera.getPosition());

    glClearColor(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

    for (unsigned int i = 0; i < 6; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadows.pointFramebuffer.getId());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               g_cameraDirections[i].cubemapFace, m_shadows.pointCube->getId(), 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glViewport(0, 0, resolution, resolution);

        // Send view/projection to default shader
        glm::mat4 view =
            glm::lookAt(camera.getPosition(), camera.getPosition() + g_cameraDirections[i].target,
                        g_cameraDirections[i].up);

        shadowCubePassShader->setUniform(projectionMatrixUniformName, camera.getProjection());
        shadowCubePassShader->setUniform(viewMatrixUniformName, view);

        // Query visible scene objects
        CSceneQuery query;
        CStaticCamera faceCamera(view, camera.getProjection(), camera.getPosition());
        scene.getVisibleObjects(faceCamera, query);

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
                CMesh *mesh = manager.getMesh(meshId);
                CMaterial *material = manager.getMaterial(materialId);
                if (material && (material->settings.background ||
                                 material->settings.alphaMode == AlphaMode::Blend))
                    continue;

                CTransformer transformer;

                // Set transformations
                transformer.setPosition(position);
                transformer.setRotation(rotation);
                transformer.setScale(scale);

                // Forward draw call
                drawMaterialMesh(mesh, transformer.getTranslationMatrix(),
                                 transformer.getRotationMatrix(), transformer.getScaleMatrix(),
                                 material, manager, shadowCubePassShader);
            }
        }
    }

    // Post draw error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
    }

    // Disable buffer
    m_shadows.pointFramebuffer.setInactive(GL_FRAMEBUFFER);

    glClearColor(0, 0, 0, 0);
    glCullFace(GL_BACK);
}

bool ShadowPass::initShadowMapPass(IResourceManager &manager)
{
    std::string shaderFile("data/shader/shadow_map_pass.ini");
    m_shadowMapPassShaderId = manager.loadShader(shaderFile);

    if (m_shadowMapPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
        return false;
    }

    // Depth texture
    m_shadows.directionalDepth = std::make_shared<CTexture>();
    m_shadows.directionalDepth->init(1, 1, GL_DEPTH_COMPONENT24);

    // Set to PCF parameters
    glBindTexture(GL_TEXTURE_2D, m_shadows.directionalDepth->getId());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    // Framebuffer attachments
    m_shadows.directionalFramebuffer.attach(m_shadows.directionalDepth, GL_DEPTH_ATTACHMENT);
    m_shadows.directionalFramebuffer.setActive(GL_FRAMEBUFFER);
    glDrawBuffer(GL_NONE);
    m_shadows.directionalFramebuffer.setInactive(GL_FRAMEBUFFER);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    SPDLOG_INFO("Shadow map buffer state: {}.",
                m_shadows.directionalFramebuffer.getState().c_str());

    // Reset framebuffer
    m_shadows.directionalFramebuffer.setInactive(GL_FRAMEBUFFER);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }
    return true;
}

bool ShadowPass::initShadowCubePass(IResourceManager &manager)
{
    std::string shaderFile("data/shader/shadow_cube_pass.ini");
    m_shadowCubePassShaderId = manager.loadShader(shaderFile);

    if (m_shadowCubePassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
        return false;
    }

    m_shadows.pointFramebuffer.resize(1024, 1024);

    // Depth texture
    m_shadows.pointDepth = std::make_shared<CTexture>();
    m_shadows.pointDepth->init(1, 1, GL_DEPTH_COMPONENT24);

    // TODO Cleanup and move into cube texture class
    glBindTexture(GL_TEXTURE_2D, m_shadows.pointDepth->getId());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R32F, 1, 1, 0, GL_RED, GL_FLOAT,
                     NULL);
    }

    m_shadows.pointCube =
        std::make_shared<CTexture>(textureId, false, 1, 1, GL_R32F, GL_RED, GL_TEXTURE_CUBE_MAP);

    // Error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadows.pointFramebuffer.getId());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           m_shadows.pointDepth->getId(), 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    m_shadows.pointFramebuffer.setInactive(GL_FRAMEBUFFER);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    SPDLOG_INFO("Shadow cube buffer state: {}.", m_shadows.pointFramebuffer.getState().c_str());

    // Reset framebuffer
    m_shadows.pointFramebuffer.setInactive(GL_FRAMEBUFFER);

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }
    return true;
}

bool ShadowPass::init(IResourceManager &manager)
{
    return initShadowMapPass(manager) && initShadowCubePass(manager);
}
