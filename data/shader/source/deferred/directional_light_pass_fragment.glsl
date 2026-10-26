#version 460 core
#include "lighting.glsl"
layout(location=0) out vec4 light_data;
layout(location=1) out vec3 specular_data;
uniform vec3 light_direction,light_color,camera_position;
uniform float light_intensity,screen_width,screen_height;
uniform mat4 inverse_view_projection,shadow_view_projection_bias;
uniform sampler2D depth_texture,normal_specular_texture;
uniform sampler2DShadow shadow_map;
uniform bool shadows_enabled=true;
uniform float shadow_bias=0.0005;
void main()
{
    vec2 uv=gl_FragCoord.xy/vec2(screen_width,screen_height);
    float depth=texture(depth_texture,uv).r;
    if(depth>=1.0) {light_data=vec4(0); specular_data=vec3(0); return;}
    vec4 world=inverse_view_projection*vec4(uv*2.0-1.0,depth*2.0-1.0,1.0);
    vec3 position=world.xyz/world.w;
    vec4 surface=texture(normal_specular_texture,uv);
    vec4 projected=shadow_view_projection_bias*vec4(position,1);
    vec3 coord=projected.xyz/projected.w;
    float visibility=1.0;
    if(shadows_enabled && all(greaterThanEqual(coord,vec3(0))) && all(lessThanEqual(coord,vec3(1)))) {
        float bias=shadow_bias*(1.0+2.0*(1.0-max(dot(safeDirection(surface.xyz),-light_direction),0.0)));
        vec2 texel=1.0/vec2(textureSize(shadow_map,0));
        visibility=0.0;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y) visibility+=texture(shadow_map,vec3(coord.xy+vec2(x,y)*texel,coord.z-bias))/9.0;
    }
    vec3 diffuse,specular;
    surfaceLight(surface.xyz,camera_position-position,-light_direction,light_color*light_intensity*visibility,surface.a,diffuse,specular);
    light_data=vec4(diffuse,0); specular_data=specular;
}
