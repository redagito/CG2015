#version 460 core
#include "lighting.glsl"
layout(location=0) out vec4 light_data;
layout(location=1) out vec3 specular_data;
uniform vec3 light_position, light_color, camera_position;
uniform float light_radius, light_intensity;
uniform float screen_width, screen_height;
uniform mat4 inverse_view_projection;
uniform sampler2D depth_texture, normal_specular_texture;
uniform samplerCube shadow_cube;
uniform bool shadows_enabled = true;
void main()
{
    vec2 uv=gl_FragCoord.xy/vec2(screen_width,screen_height);
    float depth=texture(depth_texture,uv).r;
    if(depth>=1.0) {light_data=vec4(0); specular_data=vec3(0); return;}
    vec4 world=inverse_view_projection*vec4(uv*2.0-1.0,depth*2.0-1.0,1.0);
    vec3 position=world.xyz/world.w;
    vec3 delta=light_position-position;
    float distance=length(delta);
    float attenuation=max(0.0,1.0-distance/max(light_radius,1e-6));
    float visibility=1.0;
    if(shadows_enabled && distance>=texture(shadow_cube,-delta).r+0.001) visibility=0.0;
    vec4 surface=texture(normal_specular_texture,uv);
    vec3 diffuse,specular;
    surfaceLight(surface.xyz,camera_position-position,delta,light_color*light_intensity*attenuation*visibility,surface.a,diffuse,specular);
    light_data=vec4(diffuse,0.0); specular_data=specular;
}
