#pragma once

#include <list>
#include <memory>

#include "graphics/IGraphicsSystem.h"
#include "graphics/IScene.h"

class CGraphicsSystem : public IGraphicsSystem
{
public:
	IScene* createScene();
	
private:
	std::list<std::unique_ptr<IScene>> m_scenes;
	IScene* m_activeScene = nullptr;
};