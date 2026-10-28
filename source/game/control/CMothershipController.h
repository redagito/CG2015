#pragma once
#include "game/IController.h"
#include "game/arena/MothershipTuning.h"
#include <functional>
#include <glm/glm.hpp>

// One clock drives movement, the HUD countdown and the defeat deadline.
class CMothershipController : public IController
{
  public:
    CMothershipController(glm::vec3 destination, const MothershipTuning &tuning,
                          std::function<void()> onDeadline);
    void attach(CGameObject *object) override;
    void detach() override;
    void update(float seconds) override;
    void setActive(bool active) override;
    void receiveMessage(Message message) override;
    float approachRemaining() const;
    float totalRemaining() const;

  private:
    CGameObject *m_object = nullptr;
    glm::vec3 m_start{0}, m_destination{0};
    MothershipTuning m_tuning;
    std::function<void()> m_onDeadline;
    double m_elapsed = 0;
    bool m_active = true, m_finished = false;
};
