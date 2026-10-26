#version 460 core

// Material textures
uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;
uniform sampler2D specular_texture;
uniform sampler2D glow_texture;

// Actually not needed here
uniform sampler2D alpha_texture;

uniform vec3 light_position;

in vec4 world_pos;
in vec2 uv;
uniform int alpha_mode=0;
uniform float alpha_cutoff=0.5,opacity=1.0;

layout(location = 0) out float fragmentColor;

void main(void) {
    if(alpha_mode==1 && texture(alpha_texture,uv).r*opacity<alpha_cutoff) discard;
    vec3 l2v = world_pos.xyz - light_position;
    float d = length(l2v);
    fragmentColor = d;
}
