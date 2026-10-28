#pragma once
#include "game/GameObjectId.h"
#include "game/IController.h"
class CGameWorld;
class CFollowObjectController : public IController
{
  public:
    CFollowObjectController(const CGameWorld &world, GameObjectId target);
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;

  private:
    const CGameWorld &m_world;
    GameObjectId m_target;
    CGameObject *m_object = nullptr;
    float m_angle = 0;
    bool m_active = true;
};
