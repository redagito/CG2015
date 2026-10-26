#pragma once
#include "input/IInputProvider.h"

class TestInput : public IInputProvider
{
  public:
    bool enter = false;
    bool space = false;
    void addInputListener(IInputListener *) override;
    void removeInputListener(IInputListener *) override;
    bool isKeyPressed(int key) override;
};
