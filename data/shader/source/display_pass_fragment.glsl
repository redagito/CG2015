#version 460 core

// Final scene texture
uniform sampler2D scene_texture;
uniform bool encode_output=false;

// Screen size
uniform float screen_width;
uniform float screen_height;

// Lit fragment
out vec3 color;

vec3 linearToSrgb(vec3 value) { value=max(value,vec3(0)); return mix(value*12.92,1.055*pow(value,vec3(1.0/2.4))-0.055,greaterThan(value,vec3(0.0031308))); }
void main(void)
{
	// Get screen position for uv texture lookup
	vec2 normalized_screen_coordinates = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
	// Write texture to screen
	vec3 linear=texture(scene_texture, normalized_screen_coordinates).rgb;
    color = encode_output ? linearToSrgb(linear) : linear;
}
