#version 460 core
in vec2 uv;
layout(location=0) out vec3 color;
uniform sampler2D diffuse_texture;
uniform bool has_texture=false,sun_active=false;
uniform vec2 sun_position;
uniform vec3 sun_color;
uniform float screen_width,screen_height;
void main() {
    vec2 screen=gl_FragCoord.xy/vec2(screen_width,screen_height);
    color=has_texture?texture(diffuse_texture,uv).rgb:mix(vec3(0.015,0.02,0.035),vec3(0.06,0.10,0.16),screen.y);
    vec2 delta=screen-sun_position; delta.x*=screen_width/screen_height;
    if(sun_active) color+=sun_color*12.0*(1.0-smoothstep(0.008,0.015,length(delta)));
}
