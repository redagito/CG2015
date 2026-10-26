#version 460 core
in vec2 texture_uv;
in vec4 color;
in float is_glyph;
uniform sampler2D font_texture;
layout(location=0) out vec4 result;
void main() {
    float coverage=is_glyph>0.5?texture(font_texture,texture_uv).r:1.0;
    result=vec4(color.rgb,color.a*coverage);
}
