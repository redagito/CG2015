#pragma once

class IGlfwWindowListener
{
  public:
    virtual void handleResizeEvent(int width, int height) = 0;
};
