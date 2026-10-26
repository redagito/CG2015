#pragma once

#include <list>
#include <memory>

#include "graphics/IGraphicsSystem.h"

#include "debug/CDebugInfo.h"

#include "graphics/IScene.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/IRenderer.h"
#include "graphics/CDebugInfoDisplay.h"

#include "SGraphicsSettings.h"
#include "graphics/ui/CUiRenderer.h"

class CGraphicsSystem : public IGraphicsSystem
{
public:
	CGraphicsSystem();
	~CGraphicsSystem();

	bool init(IResourceManager& manager);
	void setActiveRenderer(const std::string& renderer);

	IScene* createScene();
	void setActiveScene(IScene* scene);

	ICamera* createCamera();
	void setActiveCamera(ICamera* camera);

	void toggleDebugOverlay();
	void toggleDebugOverlayTransparency();
	void toggleViewFrustumCulling();

	void draw(IWindow& window);
    void setUiOverlay(const UiDocument& document) override { m_uiDocument=document; }

private:
    CUiRenderer m_uiRenderer;
    UiDocument m_uiDocument;
	bool m_viewFrustumClunningMode = true; /**< View frustum culling mode flag. */

	bool m_drawDebugOverlay = false;
	bool m_transparentDebugOverlay = true;
	CDebugInfo m_debugInfo; /**< Debug info data. */
	std::unique_ptr<CDebugInfoDisplay> m_debugInfoDisplay = nullptr; /**< Debug info overlay renderer. */

	// Debug info
	double m_lastCallTime = 0.f;
	double m_timeAccum = 0.f; /**< Time accumulator. */
	unsigned int m_currentFrameCount = 0;
	unsigned int m_lastFrameCount = 0;

	std::list<std::unique_ptr<IScene>> m_scenes;
	IScene* m_activeScene = nullptr;

	std::list<std::unique_ptr<ICamera>> m_cameras;
	ICamera* m_activeCamera = nullptr;

	std::unique_ptr<IRenderer> m_deferredRenderer = nullptr;
	IRenderer* m_activeRenderer = nullptr;

	std::unique_ptr<IGraphicsResourceManager> m_resourceManager = nullptr;
};
