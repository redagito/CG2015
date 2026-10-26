#version 460 core
layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;
layout(location=2) in vec4 tint;
layout(location=3) in float glyph;
uniform vec2 viewport;
out vec2 texture_uv;
out vec4 color;
out float is_glyph;
void main() {
    gl_Position=vec4(position.x/viewport.x*2.0-1.0,1.0-position.y/viewport.y*2.0,0,1);
    texture_uv=uv;color=tint;is_glyph=glyph;
}
