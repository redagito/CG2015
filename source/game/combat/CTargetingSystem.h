#pragma once
#include "TargetingSolution.h"
#include "TargetingTuning.h"
#include "WeaponAim.h"
#include <vector>
class IInputProvider;
class CTargetingSystem
{
  public:
    explicit CTargetingSystem(const TargetingTuning &tuning);
    void update(const std::vector<TargetSnapshot> &targets, glm::vec3 position, glm::vec3 forward,
                const WeaponAim &aim, IInputProvider &input);
    const std::optional<TargetingSolution> &solution() const;

  private:
    TargetingTuning m_tuning;
    GameObjectId m_selected = noGameObject;
    bool m_initialAcquisition = true, m_tabDown = false, m_selectDown = false, m_clearDown = false;
    std::optional<TargetingSolution> m_solution;
};
