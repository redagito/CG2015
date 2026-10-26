#pragma once
#include "RendererCoreConfig.h"
#include <chrono>
#include <deque>
#include <map>
#include <string>

class CRenderProfiler
{
    struct Pending { GLuint begin, end; std::string name; };
    std::deque<Pending> pending;
    std::map<std::string,double> gpu, cpu;
public:
    CRenderProfiler() = default;
    CRenderProfiler(const CRenderProfiler&) = delete;
    ~CRenderProfiler() { for(const auto& p:pending) { const GLuint ids[]={p.begin,p.end}; glDeleteQueries(2,ids); } }
    void collect()
    {
        while(!pending.empty()) {
            const auto& p=pending.front(); GLint ready=0; glGetQueryObjectiv(p.end,GL_QUERY_RESULT_AVAILABLE,&ready); if(!ready) break;
            GLuint64 begin=0,end=0; glGetQueryObjectui64v(p.begin,GL_QUERY_RESULT,&begin); glGetQueryObjectui64v(p.end,GL_QUERY_RESULT,&end);
            gpu[p.name]=static_cast<double>(end-begin)/1.e6;
            const GLuint ids[]={p.begin,p.end}; glDeleteQueries(2,ids); pending.pop_front();
        }
    }
    const auto& gpuMilliseconds() const { return gpu; }
    const auto& cpuMilliseconds() const { return cpu; }
    class Scope
    {
        CRenderProfiler& owner; std::string name; GLuint ids[2]{};
        std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
    public:
        Scope(CRenderProfiler& p, std::string label):owner(p),name(std::move(label)) { if(owner.pending.size()<256) {glGenQueries(2,ids); glQueryCounter(ids[0],GL_TIMESTAMP);} }
        ~Scope() { owner.cpu[name]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(); if(ids[0]) {glQueryCounter(ids[1],GL_TIMESTAMP); owner.pending.push_back({ids[0],ids[1],name});} }
    };
};
