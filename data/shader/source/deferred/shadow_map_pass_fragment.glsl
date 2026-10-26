#version 460 core
in vec2 uv;
uniform sampler2D alpha_texture;
uniform int alpha_mode=0;
uniform float alpha_cutoff=0.5,opacity=1.0;
void main() { if(alpha_mode==1 && texture(alpha_texture,uv).r*opacity<alpha_cutoff) discard; }
