#pragma once
#include "graphics/IWindow.h"
class CGlfwWindow;

struct TestViewport : IWindow
{
    const CGlfwWindow &context;
    unsigned width = 130;
    unsigned height = 65;
    explicit TestViewport(const CGlfwWindow &value);
    unsigned getWidth() const override;
    unsigned getHeight() const override;
    void setActive() const override;
};
