#version 460 core
#include "lighting.glsl"
#include "emissive_accents.glsl"
in vec2 uv;
in vec3 vertexWorldSpace,normalVectorWorldSpace;
layout(location=0) out vec4 color;
uniform sampler2D diffuse_texture,normal_texture,specular_texture,glow_texture,alpha_texture;
uniform samplerCube shadow_cube;
uniform sampler2DShadow shadow_map;
uniform mat4 shadow_view_projection_bias;
uniform float opacity=1.0,light_radius=1.0,shadow_bias=0.0005;
uniform vec3 camera_position,ambient_light,light_position,light_direction,light_color;
uniform bool normal_mapping_enabled=true,shadows_enabled=false;
uniform int light_kind=0;
void main() {
    float alpha=clamp(texture(alpha_texture,uv).r*opacity,0.0,1.0); if(alpha<=0.0) discard;
    vec3 base=texture(diffuse_texture,uv).rgb;
    float glow=texture(glow_texture,uv).r;
    applyEmissiveAccents(base,glow);
    if(light_kind==0) {color=vec4(base*(ambient_light+glow*4.0),alpha);return;}
    vec3 N=safeDirection(gl_FrontFacing?normalVectorWorldSpace:-normalVectorWorldSpace);
    if(normal_mapping_enabled) {
        vec3 p1=dFdx(vertexWorldSpace),p2=dFdy(vertexWorldSpace); vec2 t1=dFdx(uv),t2=dFdy(uv);
        vec3 T=cross(p2,N)*t1.x+cross(N,p1)*t2.x, B=cross(p2,N)*t1.y+cross(N,p1)*t2.y;
        float inv=inversesqrt(max(max(dot(T,T),dot(B,B)),1e-8));
        N=safeDirection(mat3(T*inv,B*inv,N)*(texture(normal_texture,uv).xyz*2.0-1.0));
    }
    vec3 L=-light_direction; float attenuation=1.0,visibility=1.0;
    if(light_kind==1) {
        L=light_position-vertexWorldSpace;float distance=length(L);
        attenuation=max(0.0,1.0-distance/max(light_radius,1e-6));
        if(shadows_enabled && distance>1e-6 && distance>=texture(shadow_cube,-L).r+0.001) visibility=0.0;
    } else if(shadows_enabled) {
        vec4 projected=shadow_view_projection_bias*vec4(vertexWorldSpace,1);vec3 coord=projected.xyz/projected.w;
        if(all(greaterThanEqual(coord,vec3(0)))&&all(lessThanEqual(coord,vec3(1)))) {
            float bias=shadow_bias*(1.0+2.0*(1.0-max(dot(N,safeDirection(L)),0.0)));
            vec2 texel=1.0/vec2(textureSize(shadow_map,0));visibility=0.0;
            for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y) visibility+=texture(shadow_map,vec3(coord.xy+vec2(x,y)*texel,coord.z-bias))/9.0;
        }
    }
    vec3 diffuse,specular;
    surfaceLight(N,camera_position-vertexWorldSpace,L,light_color*attenuation*visibility,texture(specular_texture,uv).r,diffuse,specular);
    color=vec4(base*diffuse+specular,alpha);
}
