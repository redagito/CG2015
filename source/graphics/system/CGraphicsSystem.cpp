#include "CGraphicsSystem.h"

#include "graphics/scene/CScene.h"

IScene* CGraphicsSystem::createScene()
{
	IScene* scene = new CScene;
	m_scenes.push_back(std::unique_ptr<IScene>(scene));
	return scene;
}