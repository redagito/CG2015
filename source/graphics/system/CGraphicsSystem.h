#pragma once

#include <list>
#include <memory>

#include "graphics/IGraphicsSystem.h"

#include "graphics/IScene.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/IRenderer.h"

class CGraphicsSystem : public IGraphicsSystem
{
public:
	bool init(IResourceManager& manager);
	void setActiveRenderer(const std::string& renderer);

	IScene* createScene();
	void setActiveScene(IScene* scene);

	ICamera* createCamera();
	void setActiveCamera(const ICamera* camera);

	void draw(IWindow& window);
	
private:
	std::list<std::unique_ptr<IScene>> m_scenes;
	IScene* m_activeScene = nullptr;

	std::list<std::unique_ptr<ICamera>> m_cameras;
	const ICamera* m_activeCamera = nullptr;

	std::unique_ptr<IRenderer> m_deferredRenderer = nullptr;
	std::unique_ptr<IRenderer> m_forwardRenderer = nullptr;
	IRenderer* m_activeRenderer = nullptr;

	std::unique_ptr<IGraphicsResourceManager> m_resourceManager = nullptr;
};