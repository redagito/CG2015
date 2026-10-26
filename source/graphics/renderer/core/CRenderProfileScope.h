#pragma once
#include "RendererCoreConfig.h"
#include <chrono>
#include <string>
class CRenderProfiler;

class CRenderProfileScope
{
  public:
    CRenderProfileScope(CRenderProfiler &profiler, std::string label);
    ~CRenderProfileScope();
    CRenderProfileScope(const CRenderProfileScope &) = delete;

  private:
    CRenderProfiler &owner;
    std::string name;
    GLuint ids[2]{};
    std::chrono::steady_clock::time_point start;
};
