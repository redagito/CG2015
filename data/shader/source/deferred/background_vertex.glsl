#version 460 core
layout(location=0) in vec3 vertexPositionModelSpace;
layout(location=2) in vec2 vertexUV;
uniform mat4 model,view,projection;
uniform bool fullscreen=false;
out vec2 uv;
void main() { uv=vertexUV; vec4 clip=projection*view*model*vec4(vertexPositionModelSpace,1); gl_Position=fullscreen?vec4(vertexPositionModelSpace.xy,1,1):clip.xyww; }
