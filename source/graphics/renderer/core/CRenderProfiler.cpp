#include "CRenderProfiler.h"

CRenderProfiler::CRenderProfiler() = default;
CRenderProfiler::~CRenderProfiler()
{
    for (const auto &query : pending)
    {
        const GLuint ids[] = {query.begin, query.end};
        glDeleteQueries(2, ids);
    }
}

void CRenderProfiler::collect()
{
    while (!pending.empty())
    {
        const auto &query = pending.front();
        GLint ready = 0;
        glGetQueryObjectiv(query.end, GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready)
            break;
        GLuint64 begin = 0, end = 0;
        glGetQueryObjectui64v(query.begin, GL_QUERY_RESULT, &begin);
        glGetQueryObjectui64v(query.end, GL_QUERY_RESULT, &end);
        gpu[query.name] = static_cast<double>(end - begin) / 1.e6;
        const GLuint ids[] = {query.begin, query.end};
        glDeleteQueries(2, ids);
        pending.pop_front();
    }
}

const std::map<std::string, double> &CRenderProfiler::gpuMilliseconds() const
{
    return gpu;
}

const std::map<std::string, double> &CRenderProfiler::cpuMilliseconds() const
{
    return cpu;
}
