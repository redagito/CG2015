#include "CompositionPass.h"
#include "MaterialDraw.h"

#include <algorithm>

#include <cassert>
#include <string>

#include <glm/ext.hpp>

#include "graphics/IScene.h"
#include "graphics/scene/CSceneQuery.h"
#include "graphics/ICamera.h"
#include "graphics/IWindow.h"

#include "resource/IResourceManager.h"
#include "graphics/camera/CStaticCamera.h"

#include "graphics/IGraphicsResourceManager.h"
#include "graphics/resource/CMaterial.h"
#include "graphics/resource/CMesh.h"
#include "graphics/resource/CTexture.h"
#include "graphics/resource/CShaderProgram.h"

#include "graphics/renderer/core/RendererCoreConfig.h"
#include "graphics/renderer/Draw.h"

#include "graphics/renderer/debug/RendererDebug.h"
#include <spdlog/spdlog.h>


void CompositionPass::render(const IScene& scene, const ICamera& camera,
                                         const IWindow& window,
                                         const IGraphicsResourceManager& manager,
                                         ISceneQuery& query)
{
    // Reset viewport
    glViewport(0, 0, window.getWidth(), window.getHeight());
    m_sceneColor.framebuffer.resize(window.getWidth(), window.getHeight());
    // TODO Clear illumination pass buffer?

    // Get illumination shader
    CShaderProgram* illuminationShader = manager.getShaderProgram(m_illuminationPassShaderId);
    if (illuminationShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for illumination pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for illumination pass could not be retrieved.");
        return;
    }

    // Set textures
    // L-buffer light texture
    m_lighting.diffuse->setActive(illuminationPassLightTextureUnit);
    illuminationShader->setUniform(lightTextureUniformName, illuminationPassLightTextureUnit);

    m_lighting.specular->setActive(3);
    illuminationShader->setUniform("specular_light_texture",3);

    // G-buffer diffuse glow texture
    m_gbuffer.albedoGlow->setActive(illuminationPassDiffuseGlowTextureUnit);
    illuminationShader->setUniform(diffuseGlowTextureUniformName,
                                   illuminationPassDiffuseGlowTextureUnit);

    // G-buffer depth texture
    m_gbuffer.depth->setActive(illuminationPassDepthTextureUnit);
    illuminationShader->setUniform(depthTextureUniformName, illuminationPassDepthTextureUnit);

    // Screen parameters
    illuminationShader->setUniform(screenWidthUniformName, (float)window.getWidth());
    illuminationShader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Draw into frame buffer
    m_sceneColor.framebuffer.setActive(GL_FRAMEBUFFER);
    ::draw(*quadMesh);
    m_sceneColor.framebuffer.setInactive(GL_FRAMEBUFFER);
}

bool CompositionPass::init(IResourceManager& manager)
{
    // Error check
    std::string error;
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }
    // Load illumination shader
    std::string illuminationPassShaderFile = "data/shader/deferred/illumination_pass.ini";
    m_illuminationPassShaderId = manager.loadShader(illuminationPassShaderFile);
    // Check if ok
    if (m_illuminationPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                  illuminationPassShaderFile.c_str());
        return false;
    }

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    // Screen quad mesh
    std::string quadMesh = "data/mesh/screen_quad.obj";
    m_fullscreen.quad = manager.loadMesh(quadMesh);
    // Check if ok
    if (m_fullscreen.quad == invalidResource)
    {
        SPDLOG_ERROR("Failed to load screen quad mesh {}.", quadMesh.c_str());
        return false;
    }

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    // FBO
    m_sceneColor.color = std::make_shared<CTexture>();
    if (!m_sceneColor.color->init(800, 600, GL_RGB16F))
    {
        SPDLOG_ERROR("Failed to initialize illumination pass texture.");
        return false;
    }
    m_sceneColor.framebuffer.attach(m_sceneColor.color, GL_COLOR_ATTACHMENT0);
    m_sceneColor.framebuffer.attach(m_gbuffer.depth,GL_DEPTH_ATTACHMENT);

    // TODO Check FBO

    // Error check
    if (hasGLError(error))
    {
        SPDLOG_ERROR("GL Error: {}", error.c_str());
        return false;
    }

    return true;
}
