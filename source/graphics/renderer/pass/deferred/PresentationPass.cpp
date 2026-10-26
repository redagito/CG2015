#include "PresentationPass.h"
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


void PresentationPass::displayPass(const IWindow& window, const IGraphicsResourceManager& manager,
                                    const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "display");

    // Set main FBO active
    CFrameBuffer::setDefaultActive();
    passthroughPass(window, manager, texture, true);
}

void PresentationPass::passthroughPass(const IWindow& window,
                                        const IGraphicsResourceManager& manager,
                                        const std::shared_ptr<CTexture>& texture, bool encodeOutput)
{
    CRenderProfiler::Scope timing(m_profiler, "post.copy");

    // Reset viewport
    glViewport(0, 0, window.getWidth(), window.getHeight());

    // Get display shader
    CShaderProgram* displayShader = manager.getShaderProgram(m_displayPassShaderId);
    if (displayShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for display pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for illumination pass could not be retrieved.");
        return;
    }

    // Scene texture
    texture->setActive(displayPassSceneTextureUnit);
    displayShader->setUniform(sceneTextureUniformName, displayPassSceneTextureUnit);

    displayShader->setUniform("encode_output",static_cast<int>(encodeOutput));
    // Screen parameters
    displayShader->setUniform(screenWidthUniformName, (float)window.getWidth());
    displayShader->setUniform(screenHeightUniformName, (float)window.getHeight());

    ::draw(*quadMesh);
}

void PresentationPass::visualizeDepthPass(const ICamera& camera, const IWindow& window,
                                           const IGraphicsResourceManager& manager)
{
    CRenderProfiler::Scope timing(m_profiler, "display.depth");

    // Get shader
    CShaderProgram* shader = manager.getShaderProgram(m_visualizeDepthPassShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for depth visualization pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for depth visualization pass could not be retrieved.");
        return;
    }

    // Depth texture
    m_gbuffer.depth->setActive(visualizeDepthPassDepthTextureUnit);
    shader->setUniform(depthTextureUniformName, visualizeDepthPassDepthTextureUnit);

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Camera parameters
    // TODO Replace
    shader->setUniform(cameraZNearUniformName, 0.01f);
    shader->setUniform(cameraZFarUniformName, 1000.f);

    // Perform pass
    ::draw(*quadMesh);
}

bool PresentationPass::initDisplayPass(IResourceManager& manager)
{
    // Display shader
    std::string displayPassShaderFile = "data/shader/display_pass.ini";
    m_displayPassShaderId = manager.loadShader(displayPassShaderFile);
    // Check if ok
    if (m_displayPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", displayPassShaderFile.c_str());
        return false;
    }
    return true;
}

bool PresentationPass::initVisualizeDepthPass(IResourceManager& manager)
{
    // Get shader
    std::string shaderFile = "data/shader/debug/visualize_depth_buffer_pass.ini";
    m_visualizeDepthPassShaderId = manager.loadShader(shaderFile);
    // Check if ok
    if (m_visualizeDepthPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
        return false;
    }
    return true;
}

bool PresentationPass::init(IResourceManager& manager)
{
    return initDisplayPass(manager) && initVisualizeDepthPass(manager);
}

void PresentationPass::render(const ICamera& camera, const IWindow& window,
    const IGraphicsResourceManager& manager, const DeferredRenderTargets& targets)
{
    CFrameBuffer::setDefaultActive();
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    switch (camera.getFeatureInfo().renderMode) {
    case RenderMode::Color: displayPass(window, manager, targets.gbuffer.albedoGlow); break;
    case RenderMode::Depth: visualizeDepthPass(camera, window, manager); break;
    case RenderMode::Lights: displayPass(window, manager, targets.lighting.diffuse); break;
    case RenderMode::Normals: displayPass(window, manager, targets.gbuffer.normalSpecular); break;
    default: displayPass(window, manager, targets.post.output); break;
    }
}
