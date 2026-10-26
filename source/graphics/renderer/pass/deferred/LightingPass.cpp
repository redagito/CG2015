#include "LightingPass.h"
#include "MaterialDraw.h"
#include "graphics/renderer/CRenderBuffer.h"

#include "graphics/renderer/core/ShadowMath.h"
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

void LightingPass::render(const IScene &scene, const ICamera &camera, const IWindow &window,
                          const IGraphicsResourceManager &manager, ISceneQuery &query)
{
    m_shadows.renderedDirectional.clear();
    m_shadows.renderedPoint.clear();
    if (!m_frame.hasBlendedObjects)
    {
        m_shadows.directional.clear();
        m_shadows.point.clear();
    }

    // Prepare light pass frame buffer
    glViewport(0, 0, window.getWidth(), window.getHeight());
    // Resize
    m_lighting.framebuffer.resize(window.getWidth(), window.getHeight());
    // Enable light buffer
    m_lighting.framebuffer.setActive(GL_FRAMEBUFFER);

    // Retrieve ambient light
    glm::vec3 ambientColor;
    float ambientIntensity;
    scene.getAmbientLight(ambientColor, ambientIntensity);
    // Initialize light buffer with ambient light
    glClearColor(ambientColor.x * ambientIntensity, ambientColor.y * ambientIntensity,
                 ambientColor.z * ambientIntensity, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const GLfloat noSpecular[] = {0.f, 0.f, 0.f, 0.f};
    glClearBufferfv(GL_COLOR, 1, noSpecular);

    // No depth testing for light volumes
    glDisable(GL_DEPTH_TEST);
    // Additive blending for light accumulation
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // Draw point light volumes
    pointLightPass(scene, camera, window, manager, query);

    // Draw directional lights
    directionalLightPass(scene, camera, window, manager, query);

    // Reset state and cleanup
    glDisable(GL_BLEND);
    m_lighting.framebuffer.setInactive(GL_FRAMEBUFFER);
}

void LightingPass::pointLightPass(const IScene &scene, const ICamera &camera, const IWindow &window,
                                  const IGraphicsResourceManager &manager, ISceneQuery &query)
{
    // Point light pass
    CShaderProgram *pointLightPassShader = manager.getShaderProgram(m_pointLightPassShaderId);
    if (pointLightPassShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for point light pass could not be retrieved.");
        return;
    }

    CMesh *pointLightMesh = manager.getMesh(m_pointLightSphereId);
    if (pointLightMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for point light pass could not be retrieved.");
        return;
    }

    // Render point light volumes into light buffer
    while (query.hasNextPointLight())
    {
        // Retrieve light id
        SceneObjectId pointLightId = query.getNextPointLight();
        // Retrieve light parameters
        glm::vec3 position;
        glm::vec3 color;
        float intensity;
        float radius;
        bool castsShadow;

        if (!scene.getPointLight(pointLightId, position, radius, color, intensity, castsShadow))
        {
            SPDLOG_ERROR("Failed to retrieve point light data from point light id {}.",
                         pointLightId);
        }
        else
        {
            if (radius <= 0.f || intensity <= 0.f)
                continue;
            const bool shadows = castsShadow && camera.getFeatureInfo().shadowsActive;
            // A cube face spans exactly 90 degrees. GLM accepts radians.
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.f), 1.0f, 0.01f, radius * 1.5f);
            CStaticCamera shadowCamera(glm::mat4(), shadowProj, position);
            if (shadows && m_frame.hasBlendedObjects)
            {
                auto &cached = m_shadows.point[pointLightId];
                if (!cached.cube)
                {
                    cached.depth = std::make_shared<CTexture>();
                    cached.depth->init(1, 1, GL_DEPTH_COMPONENT24);
                    GLuint id = 0;
                    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id);
                    for (int face = 0; face < 6; ++face)
                    {
                        GLint previous = 0;
                        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previous);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
                        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_R32F, 1, 1, 0,
                                     GL_RED, GL_FLOAT, nullptr);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, previous);
                    }
                    cached.cube = std::make_shared<CTexture>(id, false, 1, 1, GL_R32F, GL_RED,
                                                             GL_TEXTURE_CUBE_MAP);
                    cached.cube->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    cached.cube->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    cached.cube->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    cached.cube->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    cached.cube->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                }
                m_shadows.pointCube = cached.cube;
                m_shadows.pointDepth = cached.depth;
                glNamedFramebufferTexture(m_shadows.pointFramebuffer.getId(), GL_DEPTH_ATTACHMENT,
                                          m_shadows.pointDepth->getId(), 0);
                m_shadows.renderedPoint.insert(pointLightId);
            }
            shadowCamera.getFeatureInfoForWrite().pointShadowResolution =
                std::clamp(camera.getFeatureInfo().pointShadowResolution, 64u, 2048u);
            if (shadows)
            {
                CRenderProfileScope timer(m_profiler,
                                          "shadow.point." + std::to_string(pointLightId));
                m_shadowPass.renderPoint(scene, shadowCamera, window, manager);
            }
            pointLightPassShader->setUniform("shadows_enabled", static_cast<int>(shadows));

            // Prepare light pass frame buffer
            glViewport(0, 0, window.getWidth(), window.getHeight());
            m_lighting.framebuffer.setActive(GL_FRAMEBUFFER);

            // No depth testing for light volumes
            glDisable(GL_DEPTH_TEST);
            // Additive blending for light accumulation
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);

            // Cull front facing faces
            glCullFace(GL_FRONT);

            // Set textures for point light pass
            // Set depth texture
            m_gbuffer.depth->setActive(lightPassDepthTextureUnit);
            pointLightPassShader->setUniform(depthTextureUniformName, lightPassDepthTextureUnit);

            // Set texture with world space normal and specular power
            m_gbuffer.normalSpecular->setActive(lightPassNormalSpecularTextureUnit);
            pointLightPassShader->setUniform(normalSpecularTextureUniformName,
                                             lightPassNormalSpecularTextureUnit);

            // Set shadow texture for shadow mapping
            glActiveTexture(GL_TEXTURE0 + lightPassShadowMapTextureUnit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_shadows.pointCube->getId());
            pointLightPassShader->setUniform(shadowCubeTextureUniformName,
                                             lightPassShadowMapTextureUnit);

            // Set screen size
            pointLightPassShader->setUniform(screenWidthUniformName, (float)window.getWidth());
            pointLightPassShader->setUniform(screenHeightUniformName, (float)window.getHeight());

            // Camera position
            pointLightPassShader->setUniform(cameraPositionUniformName, camera.getPosition());

            // Inverse view-projection
            pointLightPassShader->setUniform(inverseViewProjectionMatrixUniformName,
                                             m_frame.transform.getInverseViewProjectionMatrix());

            m_frame.transform.setPosition(position);
            // Scale is calculated from light radius
            // Sphere model has radius 1.f
            m_frame.transform.setScale(glm::vec3(radius));
            // Point lights do not have rotation
            m_frame.transform.setRotation(glm::quat(1.f, 0.f, 0.f, 0.f));

            // Light volume transformation for vertex shader
            pointLightPassShader->setUniform(modelViewProjectionMatrixUniformName,
                                             m_frame.transform.getModelViewProjectionMatrix());

            // Set point light parameters for fragment shader
            pointLightPassShader->setUniform(lightPositionUniformName, position);
            pointLightPassShader->setUniform(lightRadiusUniformName, radius);
            pointLightPassShader->setUniform(lightColorUniformName, color);
            pointLightPassShader->setUniform(lightIntensityUniformName, intensity);
            ::draw(*pointLightMesh);
        }
    }

    return;
}

void LightingPass::directionalLightPass(const IScene &scene, const ICamera &camera,
                                        const IWindow &window,
                                        const IGraphicsResourceManager &manager, ISceneQuery &query)
{
    // Restrieve shader
    CShaderProgram *directionalLightPassShader =
        manager.getShaderProgram(m_directionalLightPassShaderId);
    if (directionalLightPassShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for directional light pass could not be retrieved.");
        return;
    }

    // Retrieve fullscreen quad mesh
    CMesh *quadMesh = manager.getMesh(m_directionalLightScreenQuadId);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for directional light pass could not be retrieved.");
        return;
    }

    // Render point light volumes into light buffer
    while (query.hasNextDirectionalLight())
    {
        // Retrieve light id
        SceneObjectId directionalLightId = query.getNextDirectionalLight();
        // Retrieve light parameters
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        bool castsShadow;

        if (!scene.getDirectionalLight(directionalLightId, direction, color, intensity,
                                       castsShadow))
        {
            SPDLOG_ERROR("Failed to retrieve directional light data from point light id {}.",
                         directionalLightId);
        }
        else
        {
            if (glm::dot(direction, direction) < 0.000001f || intensity <= 0.f)
                continue;
            direction = glm::normalize(direction);
            const bool shadows = castsShadow && camera.getFeatureInfo().shadowsActive;
            const auto resolution =
                std::clamp(camera.getFeatureInfo().shadowMapResolution, 64u, 4096u);
            const auto fitted = fitDirectionalShadow(
                camera, direction, camera.getFeatureInfo().shadowDistance, resolution);
            const auto shadowView = fitted.view, shadowProj = fitted.projection;
            CStaticCamera shadowCamera(shadowView, shadowProj, camera.getPosition());
            shadowCamera.getFeatureInfoForWrite().shadowMapResolution = resolution;
            directionalLightPassShader->setUniform("shadow_bias", fitted.bias);
            if (shadows && m_frame.hasBlendedObjects)
            {
                auto &cached = m_shadows.directional[directionalLightId];
                if (!cached.depth)
                {
                    cached.depth = std::make_shared<CTexture>();
                    cached.depth->init(1, 1, GL_DEPTH_COMPONENT24);
                    cached.depth->setParameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                    cached.depth->setParameter(GL_TEXTURE_COMPARE_FUNC, GL_LESS);
                    cached.depth->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    cached.depth->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
                m_shadows.directionalDepth = cached.depth;
                m_shadows.directionalFramebuffer.attach(cached.depth, GL_DEPTH_ATTACHMENT);
                cached.projectionBias = glm::translate(glm::mat4(1.f), glm::vec3(0.5f)) *
                                        glm::scale(glm::mat4(1.f), glm::vec3(0.5f)) * shadowProj *
                                        shadowView;
                cached.bias = fitted.bias;
                m_shadows.renderedDirectional.insert(directionalLightId);
            } // Render shadow map
            if (shadows)
            {
                CRenderProfileScope timer(m_profiler, "shadow.directional." +
                                                          std::to_string(directionalLightId));
                m_shadowPass.renderDirectional(scene, shadowCamera, window, manager);
            }
            directionalLightPassShader->setUniform("shadows_enabled", static_cast<int>(shadows));

            // Prepare light pass frame buffer
            m_lighting.framebuffer.setActive(GL_FRAMEBUFFER);
            glViewport(0, 0, window.getWidth(), window.getHeight());

            // No depth testing for light volumes
            glDisable(GL_DEPTH_TEST);
            // Additive blending for light accumulation
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);

            // Reset culling
            glCullFace(GL_BACK);

            // Set shader active
            directionalLightPassShader->setActive();

            // Set textures for point light pass
            // Set depth texture
            m_gbuffer.depth->setActive(lightPassDepthTextureUnit);
            directionalLightPassShader->setUniform(depthTextureUniformName,
                                                   lightPassDepthTextureUnit);

            // Set texture with world space normal and specular power
            m_gbuffer.normalSpecular->setActive(lightPassNormalSpecularTextureUnit);
            directionalLightPassShader->setUniform(normalSpecularTextureUniformName,
                                                   lightPassNormalSpecularTextureUnit);

            // Set shadow texture for shadow mapping
            m_shadows.directionalDepth->setActive(lightPassShadowMapTextureUnit);
            directionalLightPassShader->setUniform(shadowMapTextureUniformName,
                                                   lightPassShadowMapTextureUnit);

            // Set screen size
            directionalLightPassShader->setUniform(screenWidthUniformName,
                                                   (float)window.getWidth());
            directionalLightPassShader->setUniform(screenHeightUniformName,
                                                   (float)window.getHeight());

            // Inverse view-projection
            directionalLightPassShader->setUniform(
                inverseViewProjectionMatrixUniformName,
                m_frame.transform.getInverseViewProjectionMatrix());

            // Shadow ViewProjectionBias
            glm::mat4 shadowViewProjBiasMatrix =
                glm::mat4(0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
                          0.5f, 0.5f, 0.5f, 1.0f) *
                shadowProj * shadowView;
            directionalLightPassShader->setUniform(shadowViewProjectionBiasMatrixUniformName,
                                                   shadowViewProjBiasMatrix);

            directionalLightPassShader->setUniform(cameraPositionUniformName, camera.getPosition());
            // Set directional light parameters
            directionalLightPassShader->setUniform(lightDirectionUniformName, direction);
            directionalLightPassShader->setUniform(lightColorUniformName, color);
            directionalLightPassShader->setUniform(lightIntensityUniformName, intensity);
            ::draw(*quadMesh);
        }
    }

    // Reset culling
    glCullFace(GL_BACK);

    return;
}

bool LightingPass::initPointLightPass(IResourceManager &manager)
{
    // Point light shader
    std::string pointLightPassShaderFile("data/shader/deferred/point_light_pass.ini");
    m_pointLightPassShaderId = manager.loadShader(pointLightPassShaderFile);

    // Check if ok
    if (m_pointLightPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                     pointLightPassShaderFile.c_str());
        return false;
    }

    // Load sphere mesh for point light representation
    std::string sphereMesh = "data/mesh/sphere.obj";
    m_pointLightSphereId = manager.loadMesh(sphereMesh);
    if (m_pointLightSphereId == invalidResource)
    {
        SPDLOG_ERROR("Failed to load point light volume mesh {}.", sphereMesh.c_str());
        return false;
    }

    // Attach texture for light data
    m_lighting.diffuse = std::make_shared<CTexture>();
    if (!m_lighting.diffuse->init(800, 600, GL_RGBA16F))
    {
        SPDLOG_ERROR("Failed to create color texture for light pass frame buffer.");
        return false;
    }
    m_lighting.framebuffer.attach(m_lighting.diffuse, GL_COLOR_ATTACHMENT0);
    m_lighting.specular = std::make_shared<CTexture>();
    if (!m_lighting.specular->init(800, 600, GL_RGB16F))
        return false;
    m_lighting.framebuffer.attach(m_lighting.specular, GL_COLOR_ATTACHMENT1);

    // Attach depth buffer
    // Depth gets discarded, depth values from geometry pass are used
    std::shared_ptr<CRenderBuffer> depthBuffer = std::make_shared<CRenderBuffer>();
    if (!depthBuffer->init(800, 600, GL_DEPTH_COMPONENT))
    {
        SPDLOG_ERROR("Failed to create depth buffer for light pass frame buffer.");
        return false;
    }
    m_lighting.framebuffer.attach(depthBuffer, GL_DEPTH_ATTACHMENT);
    return true;
}

bool LightingPass::initDirectionalLightPass(IResourceManager &manager)
{
    // Uses same frame buffer as point light pass
    // Directional light shader
    std::string directionalLightPassShaderFile("data/shader/deferred/directional_light_pass.ini");
    m_directionalLightPassShaderId = manager.loadShader(directionalLightPassShaderFile);

    // Check if ok
    if (m_directionalLightPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                     directionalLightPassShaderFile.c_str());
        return false;
    }

    // Load quad mesh for directional light representation
    std::string quadMesh = "data/mesh/screen_quad.obj";
    m_directionalLightScreenQuadId = manager.loadMesh(quadMesh);
    if (m_directionalLightScreenQuadId == invalidResource)
    {
        SPDLOG_ERROR("Failed to load screen quad mesh {}.", quadMesh.c_str());
        return false;
    }

    return true;
}

bool LightingPass::init(IResourceManager &manager)
{
    return initPointLightPass(manager) && initDirectionalLightPass(manager);
}
