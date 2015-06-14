#pragma once


#include "AGameState.h"

#include "animation/CAnimationWorld.h"
#include "graphics/IScene.h"
#include "graphics/IGraphicsSystem.h"
#include "graphics/camera/IControllableCamera.h"

#include "resource/IResourceManager.h"
#include "input/IInputProvider.h"

#include "game/CGameObject.h"
#include "game/CGameWorld.h"

#include "collision/CCollisionSystem.h"
#include "collision/CAABBox.h"


class CGamePlayState : public AGameState
{
public:
	CGamePlayState();
	~CGamePlayState();

	bool init(IGraphicsSystem* graphicsSystem, IInputProvider* inputProvider, IResourceManager* resourceManager);
	void onEnter();
	bool update(float dtime);
	void onExit();
	const std::string& getNextState() const;
	void winCount();

private:
	IGraphicsSystem* m_graphicsSystem = nullptr;
	IScene* m_scene = nullptr;
	IResourceManager* m_resourceManager = nullptr;
	IInputProvider* m_inputProvider = nullptr; 
	std::shared_ptr<IControllableCamera> m_camera = nullptr;
	CCollisionSystem* m_collisionSystem = nullptr; /**< Collision system. */

	CGameObject* m_player; /**< Player object. */
	CGameObject* m_mothership; /**< Mothership object. */
	CGameObject* m_enemy; /**< Mothership object. */
	
	int m_enemyCount; /**< Enemy spawn counter. */
	int m_enemyId;
	float m_enemyTime; /**< Enemy spawn time. */
	float m_enemyXPosition; // Hack
	unsigned int m_playerGroup; /**< Player collision group. */
	unsigned int m_enemyGroup; /**< Enemy collision group. */

	ResourceId enemyShip;
	ResourceId enemyShipMaterial;

	int m_winCounter = 0;
};