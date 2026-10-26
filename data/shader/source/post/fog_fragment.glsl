#version 460 core
layout(location = 0) out vec4 output_color;
uniform sampler2D scene_texture;
uniform sampler2D depth_texture;
uniform float screen_width;
uniform float screen_height;
uniform int fog_type;
uniform mat4 inverse_projection;
uniform float fog_begin;
uniform float fog_end;
uniform float fog_density;
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(screen_width, screen_height);
    vec3 scene = texture(scene_texture, uv).rgb;
    if (fog_type == 0) { output_color = vec4(scene, 1.0); return; }
    float depth = texture(depth_texture, uv).r;
    vec4 view = inverse_projection * vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    float distance = max(0.0, -view.z / view.w);
    float visibility = 1.0;
    if (fog_type == 1)
        visibility = (fog_end - distance) / max(fog_end - fog_begin, 0.000001);
    else if (fog_type == 2)
        visibility = exp(-max(fog_density, 0.0) * distance);
    else if (fog_type == 3)
        visibility = exp(-pow(max(fog_density, 0.0) * distance, 2.0));
    output_color = vec4(mix(vec3(0.7, 0.6, 0.5), scene, clamp(visibility, 0.0, 1.0)), 1.0);
}
