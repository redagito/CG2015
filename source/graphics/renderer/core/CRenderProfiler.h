#pragma once
#include "CRenderProfileScope.h"
#include "RendererCoreConfig.h"
#include <deque>
#include <map>
#include <string>

class CRenderProfiler
{
  public:
    CRenderProfiler();
    ~CRenderProfiler();
    CRenderProfiler(const CRenderProfiler &) = delete;
    void collect();
    const std::map<std::string, double> &gpuMilliseconds() const;
    const std::map<std::string, double> &cpuMilliseconds() const;

  private:
    friend class CRenderProfileScope;
    struct Pending
    {
        GLuint begin, end;
        std::string name;
    };
    std::deque<Pending> pending;
    std::map<std::string, double> gpu, cpu;
};
