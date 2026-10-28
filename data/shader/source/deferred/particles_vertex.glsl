#version 460 core
layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 corner;
layout(location=3) in float kind;
uniform mat4 view_projection;
out vec2 uv;
out vec4 tint;
flat out int particle_kind;
void main(){gl_Position=view_projection*vec4(position,1);uv=corner;tint=color;particle_kind=int(kind+.5);}
