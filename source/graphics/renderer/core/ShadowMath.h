#pragma once
#include "graphics/ICamera.h"
#include <glm/ext.hpp>
#include <array>
#include <algorithm>
struct SFittedShadow { glm::mat4 view,projection; float bias; };
inline SFittedShadow fitDirectionalShadow(const ICamera& camera, glm::vec3 direction, float distance, unsigned resolution)
{
    const auto inverse=glm::inverse(camera.getProjection()*camera.getView());
    const auto inverseView=glm::inverse(camera.getView());
    const auto eye=glm::vec3(inverseView[3]),forward=-glm::normalize(glm::vec3(inverseView[2]));
    glm::vec3 low(1e30f),high(-1e30f);
    const auto up=std::abs(direction.y)>0.99f?glm::vec3(0,0,1):glm::vec3(0,1,0);
    const auto view=glm::lookAt(glm::vec3(0),glm::normalize(direction),up);
    for(int x:{-1,1}) for(int y:{-1,1}) {
        auto near4=inverse*glm::vec4(x,y,-1,1),far4=inverse*glm::vec4(x,y,1,1);
        glm::vec3 near=glm::vec3(near4)/near4.w,far=glm::vec3(far4)/far4.w;
        const float n=glm::dot(near-eye,forward),f=glm::dot(far-eye,forward);
        far=glm::mix(near,far,glm::clamp((std::max(distance,n+0.01f)-n)/std::max(f-n,1e-6f),0.f,1.f));
        for(const auto& point:{near,far}) {auto light=glm::vec3(view*glm::vec4(point,1));low=glm::min(low,light);high=glm::max(high,light);}
    }
    glm::vec2 half=glm::max(glm::vec2(high-low)*0.5f,glm::vec2(0.5f))+glm::vec2(1.f);
    glm::vec2 center=glm::vec2(low+high)*0.5f,texel=2.f*half/static_cast<float>(std::max(1u,resolution));
    half+=2.f*texel;texel=2.f*half/static_cast<float>(std::max(1u,resolution));
    center=glm::floor(center/texel)*texel;
    const float padding=std::max(10.f,distance);
    const float near=-high.z-padding,far=-low.z+padding;
    return {view,glm::ortho(center.x-half.x,center.x+half.x,center.y-half.y,center.y+half.y,near,far),2.f*std::max(texel.x,texel.y)/(far-near)};
}
