#pragma once
#include "input/IInputProvider.h"

class TestInput : public IInputProvider
{
  public:
    bool enter = false;
    bool space = false, fire = false, flightMode = false;
    bool throttleUp = false, throttleDown = false, rollLeft = false, rollRight = false,
         boost = false;
    glm::vec2 pointer{0}, viewport{1280, 720};
    bool isMouseButtonPressed(int button) override;
    glm::vec2 pointerOffset() override;
    glm::vec2 viewportSize() const override;
    void setFlightMode(bool enabled, float radius = .22f) override;
    void addInputListener(IInputListener *) override;
    void removeInputListener(IInputListener *) override;
    bool isKeyPressed(int key) override;
};
