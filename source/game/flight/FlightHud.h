#pragma once
#include "graphics/ui/UiDocument.h"
#include <glm/glm.hpp>
struct FlightState;
class ICamera;

class FlightHud
{
  public:
    static UiDocument build(const ICamera &camera, glm::vec3 position, const FlightState &flight,
                            glm::vec2 viewport);

  private:
    static bool project(const ICamera &camera, glm::vec3 point, glm::vec2 &anchor);
};
