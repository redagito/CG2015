#include "PostProcessPass.h"
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


void PostProcessPass::render(const ICamera& camera, const IWindow& window,
                                        const IGraphicsResourceManager& manager,
                                        const std::shared_ptr<CTexture>& texture)
{
    glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);

    // Reset viewport
    glViewport(0, 0, window.getWidth(), window.getHeight());
    // Resize frame buffer
    m_post.primary.framebuffer.resize(window.getWidth(), window.getHeight());
    m_post.secondary.framebuffer.resize(window.getWidth(), window.getHeight());
    m_post.scratch.framebuffer.resize(window.getWidth(), window.getHeight());

    // Pass 1: fxaa
    m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
    if (camera.getFeatureInfo().fxaaActive)
    {
        fxaaPass(window, manager, texture);
    }
    else
    {
        // Passthrough
        m_presentation.passthroughPass(window, manager, texture);
    }
	// Anti aliased scene texture in primary target

    // Pass 2: fog
    // TODO Fog parameter
    m_post.secondary.framebuffer.setActive(GL_FRAMEBUFFER);
    fogPass(camera, window, manager, m_post.primary.color);
    // Foggy scene in secondary target

    // Pass 3: dof
	m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
    if (camera.getFeatureInfo().dofActive)
    {
        // Pass 3.1: gauss blur
        gaussBlurVerticalPass(window, manager, m_post.secondary.color);
        // Blurred in primary target

        m_post.scratch.framebuffer.setActive(GL_FRAMEBUFFER);
        gaussBlurHorizontalPass(window, manager, m_post.primary.color);
        // Blurred in scratch target

		for (unsigned int i = 0; i < 3; ++i)
		{
			m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
			gaussBlurVerticalPass(window, manager, m_post.scratch.color);
			// Blurred in primary target

			m_post.scratch.framebuffer.setActive(GL_FRAMEBUFFER);
			gaussBlurHorizontalPass(window, manager, m_post.primary.color);
			// Blurred in scratch target
		}

		// TODO DOF parameter
        m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
        depthOfFieldPass(camera, window, manager, m_post.secondary.color,
                         m_post.scratch.color);
    }
    else
    {
        // Passthrough
		m_presentation.passthroughPass(window, manager, m_post.secondary.color);
    }
    // Processed scene in primary target

	// Godray technique, consists of 2 passes
	if ((camera.getFeatureInfo().godRayActive || camera.getFeatureInfo().renderMode == RenderMode::GodRay))
	{
		// God ray pass 1
		m_post.secondary.framebuffer.setActive(GL_FRAMEBUFFER);
		godRayPass1(window, manager, m_post.primary.color);
		// God ray texture in secondary target

		// Godray debug mode
		if (camera.getFeatureInfo().renderMode == RenderMode::GodRay)
		{
			// Return generated godray texture
			m_post.output = m_post.secondary.color;
			return;
		}

		// God ray pass 2
		m_post.scratch.framebuffer.setActive(GL_FRAMEBUFFER);
		godRayPass2(window, manager, m_post.primary.color, m_post.secondary.color);
	}
	else
	{
		// Passthrough into scratch target
		m_post.scratch.framebuffer.setActive(GL_FRAMEBUFFER);
		m_presentation.passthroughPass(window, manager, m_post.primary.color);
	}

	// Vignette blur pass
	m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
	vignetteBlurPass(window, manager, m_post.scratch.color);
	// Scene in primary target

	// Bloom pass
	m_post.secondary.framebuffer.setActive(GL_FRAMEBUFFER);
	bloomPass1(window, manager, m_post.primary.color);
	// Bloom scene in secondary target

	// Additive blend
	// Sample primary target into a separate target: reading an attached draw texture
	// is an undefined framebuffer feedback loop, even for a full-screen pass.
	m_post.scratch.framebuffer.setActive(GL_FRAMEBUFFER);
	bloomPass2(window, manager, m_post.primary.color, m_post.secondary.color);
	auto sceneWithGlare = m_post.scratch.color;

	if (camera.getFeatureInfo().lenseFlareActive)
	{
		// Lens flare pass
		m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
		lensFlarePass(window, manager, sceneWithGlare);

		m_post.secondary.framebuffer.setActive(GL_FRAMEBUFFER);
		lensFlarePass2(window, manager, m_post.primary.color);

		m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
		lensFlarePass3(window, manager, sceneWithGlare, m_post.secondary.color);
		sceneWithGlare = m_post.primary.color;
		// Scene with flare in primary target
	}

	// Tone map
	m_post.secondary.framebuffer.setActive(GL_FRAMEBUFFER);
	toneMapPass(window, manager, sceneWithGlare);
	// Tone mapped scene in secondary target

	if (camera.getFeatureInfo().toonActive)
	{
		// Cel shading pass
		m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
		celPass(window, manager, m_post.secondary.color);
	}
	else
	{
		m_post.primary.framebuffer.setActive(GL_FRAMEBUFFER);
		m_presentation.passthroughPass(window, manager, m_post.secondary.color);
	}

    // Set output texture
    m_post.output = m_post.primary.color;
	return;
}

void PostProcessPass::fxaaPass(const IWindow& window, const IGraphicsResourceManager& manager,
                                 const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.fxaa");

    // Get fxaa shader
    CShaderProgram* fxaaShader = manager.getShaderProgram(m_fxaaPassShaderId);
    if (fxaaShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for fxaa pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for fxaa pass could not be retrieved.");
        return;
    }

    // Set input texture
    texture->setActive(fxaaPassInputTextureUnit);
    fxaaShader->setUniform(sceneTextureUniformName, fxaaPassInputTextureUnit);

    // Set screen size
    fxaaShader->setUniform(screenWidthUniformName, (float)window.getWidth());
    fxaaShader->setUniform(screenHeightUniformName, (float)window.getHeight());

    ::draw(*quadMesh);
}

void PostProcessPass::fogPass(const ICamera& camera, const IWindow& window,
                                const IGraphicsResourceManager& manager,
                                const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.fog");

    // Get fog shader
    CShaderProgram* fogShader = manager.getShaderProgram(m_fogPassShaderId);
    if (fogShader == nullptr)
    {
        SPDLOG_ERROR("Shader program for fog pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for fog pass could not be retrieved.");
        return;
    }

    fogShader->setActive();

    texture->setActive(fogPassSceneTextureUnit);
    fogShader->setUniform(sceneTextureUniformName, fogPassSceneTextureUnit);

    m_gbuffer.depth->setActive(fogPassDepthTextureUnit);
    fogShader->setUniform(depthTextureUniformName, fogPassDepthTextureUnit);

    fogShader->setUniform(screenWidthUniformName, (float)window.getWidth());
    fogShader->setUniform(screenHeightUniformName, (float)window.getHeight());

    int fogType = 0;
    switch (camera.getFeatureInfo().fogType)
    {
    case FogType::None:
        fogType = 0;
        break;
    case FogType::Linear:
        fogType = 1;
        break;
    case FogType::Exp:
        fogType = 2;
        break;
    case FogType::Exp2:
        fogType = 3;
        break;
    default:
        fogType = 0;
    }
    fogShader->setUniform(fogPassTypeUniformName, fogType);
    fogShader->setUniform(inverseProjectionMatrixUniformName, glm::inverse(camera.getProjection()));
    fogShader->setUniform("fog_begin", camera.getFeatureInfo().fogBegin);
    fogShader->setUniform("fog_end", camera.getFeatureInfo().fogEnd);
    fogShader->setUniform("fog_density", camera.getFeatureInfo().fogDensity);

    ::draw(*quadMesh);
}

void PostProcessPass::gaussBlurVerticalPass(const IWindow& window,
                                              const IGraphicsResourceManager& manager,
                                              const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.blur_vertical");

    // Get gauss shader
    CShaderProgram* shader = manager.getShaderProgram(m_gaussBlurVerticalShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for vertical gauss blur pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for vertical gauss blur pass could not be retrieved.");
        return;
    }

    // Input texture
    texture->setActive(gaussBlurVerticalPassInputTextureUnit);
    shader->setUniform(texture0UniformName, gaussBlurVerticalPassInputTextureUnit);

    // Blur parameter
    shader->setUniform(blurStrengthUniformName, 2.f);

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Perform pass
    ::draw(*quadMesh);
}

void PostProcessPass::gaussBlurHorizontalPass(const IWindow& window,
                                                const IGraphicsResourceManager& manager,
                                                const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.blur_horizontal");

    // Get gauss shader
    CShaderProgram* shader = manager.getShaderProgram(m_gaussBlurHorizontalShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for horizontal gauss blur pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for horizontal gauss blur pass could not be retrieved.");
        return;
    }

    // Input texture
    texture->setActive(gaussBlurHoriontalPassInputTextureUnit);
    shader->setUniform(texture0UniformName, gaussBlurHoriontalPassInputTextureUnit);

    // Blur parameter
    shader->setUniform(blurStrengthUniformName, 2.f);

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Perform pass
    ::draw(*quadMesh);
}

void PostProcessPass::depthOfFieldPass(const ICamera& camera, const IWindow& window,
                                         const IGraphicsResourceManager& manager,
                                         const std::shared_ptr<CTexture>& sceneTexture,
                                         const std::shared_ptr<CTexture>& blurTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.dof");

    // Get shader
    CShaderProgram* shader = manager.getShaderProgram(m_depthOfFieldPassShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for depth-of-field pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for depth-of-field pass could not be retrieved.");
        return;
    }

    // Input scene texture
    sceneTexture->setActive(depthOfFieldPassSceneTextureUnit);
    shader->setUniform(sceneTextureUniformName, depthOfFieldPassSceneTextureUnit);

    // Input blur texture
    blurTexture->setActive(depthOfFieldPassBlurTextureUnit);
    shader->setUniform(blurTextureUniformName, depthOfFieldPassBlurTextureUnit);

    // Input depth texture
    m_gbuffer.depth->setActive(depthOfFieldPassDepthTextureUnit);
    shader->setUniform(depthTextureUniformName, depthOfFieldPassDepthTextureUnit);

    // Depth-of-field parameters
    shader->setUniform(blurNearUniformName, camera.getFeatureInfo().dofNearBlur);
    shader->setUniform(focusNearUniformName, camera.getFeatureInfo().dofNearFocus);
    shader->setUniform(focusFarUniformName, camera.getFeatureInfo().dofFarFocus);
    shader->setUniform(blurFarUniformName, camera.getFeatureInfo().dofFarBlur);

    // Camera position
    shader->setUniform(cameraPositionUniformName, camera.getPosition());

    // Transformation
    shader->setUniform(inverseViewProjectionMatrixUniformName,
                       m_frame.transform.getInverseViewProjectionMatrix());

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Perform pass
    ::draw(*quadMesh);
}

void PostProcessPass::godRayPass1(const IWindow& window, const IGraphicsResourceManager& manager,
                                    const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.sun_rays");

    // Get shader
    CShaderProgram* shader = manager.getShaderProgram(m_godRayPass1ShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for god ray 1 pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for god ray 1 pass could not be retrieved.");
        return;
    }

    // Input texture
    texture->setActive(godRayPass1InputTextureUnit);
    shader->setUniform(sceneTextureUniformName, godRayPass1InputTextureUnit);

    // Depth texture
    m_gbuffer.depth->setActive(godRayPass1DepthTextureUnit);
    shader->setUniform(depthTextureUniformName, godRayPass1DepthTextureUnit);

    // Inverse projection
    shader->setUniform(inverseProjectionMatrixUniformName,
                       m_frame.transform.getInverseProjectionMatrix());

    // Light position
    shader->setUniform(lightPositionScreenUniformName, m_sun.screen);
    shader->setUniform("sun_active",static_cast<int>(m_sun.visible));
    shader->setUniform("sun_color",m_sun.color);

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Perform pass
    ::draw(*quadMesh);
}

void PostProcessPass::godRayPass2(const IWindow& window, const IGraphicsResourceManager& manager,
                                    const std::shared_ptr<CTexture>& sceneTexture,
                                    const std::shared_ptr<CTexture>& godrayTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.sun_composite");

    // Get gauss shader
    CShaderProgram* shader = manager.getShaderProgram(m_godRayPass2ShaderId);
    if (shader == nullptr)
    {
        SPDLOG_ERROR("Shader program for god ray 2 pass could not be retrieved.");
        return;
    }

    // Get screen space quad
    CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
    if (quadMesh == nullptr)
    {
        SPDLOG_ERROR("Mesh object for god ray 2 pass could not be retrieved.");
        return;
    }

    // Scene texture
    sceneTexture->setActive(godRayPass2SceneTextureUnit);
    shader->setUniform(sceneTextureUniformName, godRayPass2SceneTextureUnit);

    // God ray texture
    godrayTexture->setActive(godRayPass2GodRayTextureUnit);
    shader->setUniform(godRayTextureUniformName, godRayPass2GodRayTextureUnit);

    // Screen size
    shader->setUniform(screenWidthUniformName, (float)window.getWidth());
    shader->setUniform(screenHeightUniformName, (float)window.getHeight());

    // Perform pass
    ::draw(*quadMesh);
}

void PostProcessPass::vignetteBlurPass(const IWindow& window, const IGraphicsResourceManager& manager, const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.vignette");

	// Get distortion shader
	CShaderProgram* shader = manager.getShaderProgram(m_vignetteBlurPassShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for vignette blur pass could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for vignette blur pass could not be retrieved.");
		return;
	}

	// Set input texture
	texture->setActive(vignetteBlurPassInputTextureUnit);
	shader->setUniform(sceneTextureUniformName, vignetteBlurPassInputTextureUnit);

	// Set screen size
	shader->setUniform(screenWidthUniformName, (float) window.getWidth());
	shader->setUniform(screenHeightUniformName, (float) window.getHeight());

	::draw(*quadMesh);
}

void PostProcessPass::bloomPass1(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.bloom_extract");

	// Get bloom pass 1 shader
	CShaderProgram* shader = manager.getShaderProgram(m_bloomPass1ShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for bloom 1 pass could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for bloom 1 pass could not be retrieved.");
		return;
	}

	// Set input texture
	texture->setActive(bloomPass1InputTextureUnit);
	shader->setUniform(sceneTextureUniformName, bloomPass1InputTextureUnit);

	// Set screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	::draw(*quadMesh);
}

void PostProcessPass::bloomPass2(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& sceneTexture,
	const std::shared_ptr<CTexture>& bloomTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.bloom_composite");

	// TODO Actually same as godray pass 2, create additive blend pass
	// Get bloom pass 2 shader
	CShaderProgram* shader = manager.getShaderProgram(m_bloomPass2ShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for bloom pass 2 could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for bloom pass 2 could not be retrieved.");
		return;
	}

	// Scene texture
	sceneTexture->setActive(bloomPass2SceneTextureUnit);
	shader->setUniform(sceneTextureUniformName, bloomPass2SceneTextureUnit);

	// Bloom texture
	bloomTexture->setActive(bloomPass2BloomTextureUnit);
	shader->setUniform(bloomTextureUniformName, bloomPass2BloomTextureUnit);

	// Screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	// Perform pass
	::draw(*quadMesh);
}

void PostProcessPass::lensFlarePass(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& sceneTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.flare_extract");

	CShaderProgram* shader = manager.getShaderProgram(m_lensFlarePassShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for lens flare could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for lens flare could not be retrieved.");
		return;
	}

	// Scene texture
	sceneTexture->setActive(lensFlare1InputTextureUnit);
	shader->setUniform(sceneTextureUniformName, lensFlare1InputTextureUnit);

	// Screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	// Perform pass
	::draw(*quadMesh);
}

void PostProcessPass::lensFlarePass2(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& sceneTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.flare_generate");

	CShaderProgram* shader = manager.getShaderProgram(m_lensFlarePass2ShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for lens flare could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for lens flare could not be retrieved.");
		return;
	}

	// Scene texture
	sceneTexture->setActive(lensFlare2InputTextureUnit);
	shader->setUniform(sceneTextureUniformName, lensFlare2InputTextureUnit);

	// Screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	// Perform pass
	::draw(*quadMesh);
}

void PostProcessPass::lensFlarePass3(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& sceneTexture,
	const std::shared_ptr<CTexture>& lensTexture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.flare_composite");

	CShaderProgram* shader = manager.getShaderProgram(m_lensFlarePass3ShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for lens flare could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for lens flare could not be retrieved.");
		return;
	}

	// Scene texture
	sceneTexture->setActive(lensFlare3SceneTextureUnit);
	shader->setUniform(sceneTextureUniformName, lensFlare3SceneTextureUnit);

	// Scene texture
	lensTexture->setActive(lensFlare3LensTextureUnit);
	shader->setUniform(lensFlareTextureUniformName, lensFlare3LensTextureUnit);

	// Screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	// Perform pass
	::draw(*quadMesh);
}

void PostProcessPass::celPass(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& sceneTexture)
{
	CShaderProgram* shader = manager.getShaderProgram(m_celPassShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for cel Pass could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for cel Pass could not be retrieved.");
		return;
	}

	// Scene texture
	sceneTexture->setActive(celPassInputTextureUnit);
	shader->setUniform(sceneTextureUniformName, celPassInputTextureUnit);

	// Screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	// Perform pass
	::draw(*quadMesh);
}

void PostProcessPass::toneMapPass(const IWindow& window, const IGraphicsResourceManager& manager,
	const std::shared_ptr<CTexture>& texture)
{
    CRenderProfiler::Scope timing(m_profiler, "post.tonemap");

	// Get tone map shader
	CShaderProgram* shader = manager.getShaderProgram(m_toneMapPassShaderId);
	if (shader == nullptr)
	{
		SPDLOG_ERROR("Shader program for tone map pass could not be retrieved.");
		return;
	}

	// Get screen space quad
	CMesh* quadMesh = manager.getMesh(m_fullscreen.quad);
	if (quadMesh == nullptr)
	{
		SPDLOG_ERROR("Mesh object for tone map pass could not be retrieved.");
		return;
	}

	// Set input texture
	texture->setActive(toneMapPassInputTextureUnit);
	shader->setUniform(sceneTextureUniformName, toneMapPassInputTextureUnit);

	// Set screen size
	shader->setUniform(screenWidthUniformName, (float)window.getWidth());
	shader->setUniform(screenHeightUniformName, (float)window.getHeight());

	::draw(*quadMesh);
}

bool PostProcessPass::init(IResourceManager& manager)
{
    // Gauss blur pass
    if (!initGaussBlurHorizontalPass(manager))
    {
        SPDLOG_ERROR("Failed to initialize gauss blur horizontal pass.");
        return false;
    }
    if (!initGaussBlurVerticalPass(manager))
    {
        SPDLOG_ERROR("Failed to initialize gauss blur vertical pass.");
        return false;
    }

    // FXAA
    if (!initFxaaPass(manager))
    {
        SPDLOG_ERROR("Failed to initialize fxaa pass.");
        return false;
    }

    // Fog
    if (!initFogPass(manager))
    {
        SPDLOG_ERROR("Failed to initialize fog pass.");
        return false;
    }

	// Godray
    if (!initGodRayPass1(manager))
    {
        SPDLOG_ERROR("Failed to initialize god ray 1 pass.");
        return false;
    }

	// Should actually compose by additive blending
    if (!initGodRayPass2(manager))
    {
        SPDLOG_ERROR("Failed to initialize god ray 2 pass.");
        return false;
    }

	// DOF pass
	if (!initDepthOfFieldPass(manager))
	{
		SPDLOG_ERROR("Failed to initialize depth of field pass.");
		return false;
	}

	// Vignette blur pass
	if (!initVignetteBlurPass(manager))
	{
		SPDLOG_ERROR("Failed to initialize depth visualization pass.");
		return false;
	}

	// Bloom pass 1
	if (!initBloomPass1(manager))
	{
		SPDLOG_ERROR("Failed to initialize bloom pass 1.");
		return false;
	}

	// Bloom pass 2
	if (!initBloomPass2(manager))
	{
		SPDLOG_ERROR("Failed to initialize bloom pass 2.");
		return false;
	}

	if (!initLensFlarePass(manager)) {
		SPDLOG_ERROR("Failed to initialize lens flare pass.");
		return false;
	}

	if (!initLensFlarePass2(manager)) {
		SPDLOG_ERROR("Failed to initialize lens flare pass.");
		return false;
	}

	if (!initLensFlarePass3(manager)) {
		SPDLOG_ERROR("Failed to initialize lens flare pass.");
		return false;
	}
	if (!initCelPass(manager)) {
		SPDLOG_ERROR("Failed to initialize cel pass.");
		return false;
	}

	// Tone map pass (non-adaptive)
	if (!initToneMapPass(manager))
	{
		SPDLOG_ERROR("Failed to initialize non-adaptive tonemap pass.");
		return false;
	}

    // Screen quad mesh
    std::string quadMesh = "data/mesh/screen_quad.obj";
    m_fullscreen.quad = manager.loadMesh(quadMesh);
    if (m_fullscreen.quad == invalidResource)
    {
        SPDLOG_ERROR("Failed to load screen quad mesh {}.", quadMesh.c_str());
        return false;
    }

    // Init post processing FBOs
    // Texture 0
    m_post.primary.color = std::make_shared<CTexture>();
    if (!m_post.primary.color->init(800, 600, GL_RGB16F))
    {
        SPDLOG_ERROR("Failed to initialize post process pass texture 0.");
        return false;
	}
	m_post.primary.color->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	m_post.primary.color->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	m_post.primary.color->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Texture 1
    m_post.secondary.color = std::make_shared<CTexture>();
    if (!m_post.secondary.color->init(800, 600, GL_RGB16F))
    {
        SPDLOG_ERROR("Failed to initialize post process pass texture 1.");
        return false;
	}
	m_post.secondary.color->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	m_post.secondary.color->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	m_post.secondary.color->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Texture 2
    m_post.scratch.color = std::make_shared<CTexture>();
    if (!m_post.scratch.color->init(800, 600, GL_RGB16F))
    {
        SPDLOG_ERROR("Failed to initialize post process pass texture 2.");
        return false;
	}
	m_post.scratch.color->setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	m_post.scratch.color->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	m_post.scratch.color->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    m_post.primary.framebuffer.attach(m_post.primary.color, GL_COLOR_ATTACHMENT0);
    m_post.secondary.framebuffer.attach(m_post.secondary.color, GL_COLOR_ATTACHMENT0);
    m_post.scratch.framebuffer.attach(m_post.scratch.color, GL_COLOR_ATTACHMENT0);
    // TODO Depth attachment required?
    return true;
}

bool PostProcessPass::initDepthOfFieldPass(IResourceManager& manager)
{
    // Depth of field shader
    std::string depthOfFieldShaderFile = "data/shader/post/depth_of_field_pass.ini";
    m_depthOfFieldPassShaderId = manager.loadShader(depthOfFieldShaderFile);
    // Check if ok
    if (m_depthOfFieldPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", depthOfFieldShaderFile.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initGaussBlurVerticalPass(IResourceManager& manager)
{
    // Depth of field shader
    std::string gaussBlurVerticalShaderFile = "data/shader/post/gauss_blur_vertical_pass.ini";
    m_gaussBlurVerticalShaderId = manager.loadShader(gaussBlurVerticalShaderFile);
    // Check if ok
    if (m_gaussBlurVerticalShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                  gaussBlurVerticalShaderFile.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initGaussBlurHorizontalPass(IResourceManager& manager)
{
    // Depth of field shader
    std::string gaussBlurHorizontalShaderFile = "data/shader/post/gauss_blur_horizontal_pass.ini";
    m_gaussBlurHorizontalShaderId = manager.loadShader(gaussBlurHorizontalShaderFile);
    // Check if ok
    if (m_gaussBlurHorizontalShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.",
                  gaussBlurHorizontalShaderFile.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initFxaaPass(IResourceManager& manager)
{
    // Fxaa shader
    std::string fxaaShaderFile = "data/shader/post/fxaa_pass.ini";
    m_fxaaPassShaderId = manager.loadShader(fxaaShaderFile);
    // Check if ok
    if (m_fxaaPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", fxaaShaderFile.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initFogPass(IResourceManager& manager)
{
    // Fog shader
    std::string fogShaderFile = "data/shader/post/fog_pass.ini";
    m_fogPassShaderId = manager.loadShader(fogShaderFile);
    // Check if ok
    if (m_fogPassShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", fogShaderFile.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initGodRayPass1(IResourceManager& manager)
{
    // Get shader
    std::string shader = "data/shader/post/god_ray_1_pass.ini";
    m_godRayPass1ShaderId = manager.loadShader(shader);
    // Check if ok
    if (m_godRayPass1ShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", shader.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initGodRayPass2(IResourceManager& manager)
{
    // Get shader
    std::string shader = "data/shader/post/god_ray_2_pass.ini";
    m_godRayPass2ShaderId = manager.loadShader(shader);
    // Check if ok
    if (m_godRayPass2ShaderId == invalidResource)
    {
        SPDLOG_ERROR("Failed to initialize the shader from file {}.", shader.c_str());
        return false;
    }
    return true;
}

bool PostProcessPass::initVignetteBlurPass(IResourceManager& manager)
{
	// Get shader
	std::string shaderFile = "data/shader/post/vignette_blur_pass.ini";
	m_vignetteBlurPassShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_vignetteBlurPassShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initBloomPass1(IResourceManager& manager)
{
	// Get shader
	std::string shaderFile = "data/shader/post/bloom_1_pass.ini";
	m_bloomPass1ShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_bloomPass1ShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initBloomPass2(IResourceManager& manager)
{
	// Get shader
	std::string shaderFile = "data/shader/post/bloom_2_pass.ini";
	m_bloomPass2ShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_bloomPass2ShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initLensFlarePass(IResourceManager& manager)
{
	//Get shader
	std::string shaderFile = "data/shader/post/lens_flare_pass.ini";
	m_lensFlarePassShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_lensFlarePassShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initLensFlarePass2(IResourceManager& manager)
{
	//Get shader
	std::string shaderFile = "data/shader/post/lens_flare_pass2.ini";
	m_lensFlarePass2ShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_lensFlarePass2ShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initLensFlarePass3(IResourceManager& manager)
{
	//Get shader
	std::string shaderFile = "data/shader/post/lens_flare_pass3.ini";
	m_lensFlarePass3ShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_lensFlarePass3ShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initCelPass(IResourceManager& manager)
{
	//Get shader
	std::string shaderFile = "data/shader/post/cel_pass.ini";
	m_celPassShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_celPassShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}

bool PostProcessPass::initToneMapPass(IResourceManager& manager)
{
	// Get shader
	std::string shaderFile = "data/shader/post/tonemap_pass.ini";
	m_toneMapPassShaderId = manager.loadShader(shaderFile);
	// Check if ok
	if (m_toneMapPassShaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to initialize the shader from file {}.", shaderFile.c_str());
		return false;
	}
	return true;
}
