#pragma once
#include "graphics/ui/UiDocument.h"
#include <glm/glm.hpp>
struct FlightState;
class ICamera;
struct WeaponAim;
struct TargetingSolution;
struct ArenaTuning;
struct CombatStatus;
struct PlayerFeedback;

class FlightHud
{
  public:
    static UiDocument build(const ICamera &camera, glm::vec3 position, const FlightState &flight,
                            glm::vec2 viewport, const WeaponAim *weapon = nullptr,
                            const TargetingSolution *target = nullptr,
                            const ArenaTuning *arena = nullptr,
                            const CombatStatus *status = nullptr,
                            const PlayerFeedback *feedback = nullptr);

  private:
    static bool project(const ICamera &camera, glm::vec3 point, glm::vec2 &anchor);
};
