#include "CRenderProfileScope.h"
#include "CRenderProfiler.h"

CRenderProfileScope::CRenderProfileScope(CRenderProfiler &profiler, std::string label)
    : owner(profiler), name(std::move(label)), start(std::chrono::steady_clock::now())
{
    // Queries are collected later; recording a scope never waits for the GPU.
    if (owner.pending.size() < 256)
    {
        glGenQueries(2, ids);
        glQueryCounter(ids[0], GL_TIMESTAMP);
    }
}

CRenderProfileScope::~CRenderProfileScope()
{
    owner.cpu[name] =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (ids[0])
    {
        glQueryCounter(ids[1], GL_TIMESTAMP);
        owner.pending.push_back({ids[0], ids[1], name});
    }
}
