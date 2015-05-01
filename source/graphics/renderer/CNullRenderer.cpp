#include <cassert>

#include "CNullRenderer.h"

#include "graphics/IScene.h"
#include "graphics/ICamera.h"
#include "graphics/IWindow.h"
#include "graphics/IGraphicsResourceManager.h"

#include "core/RendererCoreConfig.h"

#include "debug/RendererDebug.h"
#include "debug/Log.h"

void CNullRenderer::draw(const IScene& scene, const ICamera& camera, const IWindow& window, const IGraphicsResourceManager& manager)
{
	window.setActive();
	// Set clear color
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	return;
}

CNullRenderer* CNullRenderer::create()
{
	return new CNullRenderer;
}