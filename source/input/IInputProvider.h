#pragma once

#include "EKeyCode.h"
#include <glm/vec2.hpp>

class IInputListener;

/**
 * \brief Input provider interface.
 * Represents an abstract source. The input is handled by registered input handlers.
 */
class IInputProvider
{
  public:
    virtual ~IInputProvider();

    virtual void addInputListener(IInputListener *listener) = 0;
    virtual void removeInputListener(IInputListener *listener) = 0;

    virtual bool isKeyPressed(int keyCode) = 0;
    virtual bool isMouseButtonPressed(int button) = 0;
    virtual glm::vec2 cursorPosition() const = 0; // Window pixels, for menus.
    virtual glm::vec2 pointerOffset() = 0; // Displacement / shorter window dimension; +Y is down.
    virtual glm::vec2 viewportSize() const = 0;
    virtual void setFlightMode(bool enabled, float radius = .22f) = 0;
};
