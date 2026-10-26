#version 460 core
uniform sampler2D diffuse_glow_texture,light_texture,specular_light_texture;
uniform float screen_width,screen_height;
layout(location=0) out vec3 fragmentColor;
void main()
{
    vec2 uv=gl_FragCoord.xy/vec2(screen_width,screen_height);
    vec4 surface=texture(diffuse_glow_texture,uv);
    fragmentColor=surface.rgb*texture(light_texture,uv).rgb+texture(specular_light_texture,uv).rgb+surface.rgb*surface.a*4.0;
}
