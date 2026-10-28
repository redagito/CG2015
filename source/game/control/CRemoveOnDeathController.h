#pragma once
#include "game/IController.h"
#include <functional>
// Reports one defeat event and removes the owner; independent of encounter size or game mode.
class CRemoveOnDeathController : public IController
{
  public:
    explicit CRemoveOnDeathController(std::function<void()> onDefeat);
    ~CRemoveOnDeathController();
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;

  private:
    CGameObject *m_object = nullptr;
    bool m_active = true;
    std::function<void()> m_onDefeat;
};
