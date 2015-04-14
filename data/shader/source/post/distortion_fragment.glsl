#version 400

// Noise based distortion shader
// Radial calculation and fast noise blur based on work by Francois Tarlier and Martins Upitis
// Vignette by Thibaut Despoulain

uniform sampler2D scene_texture;
uniform float screen_width;
uniform float screen_height;

layout (location = 0) out vec3 color;

// Blur intensity
// TODO Should be blur_strength
float blurIntensity = 2.0;

// Randomization function for distortion blur
// Needed for fast noise based blurring
// TODO Noise function not properly implemented in glsl?
vec2 rand(vec2 co)
{
	float noise1 = fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
	float noise2 = fract(sin(dot(co, vec2(12.9898, 78.233) * 2.0)) * 43758.5453);
	return clamp(vec2(noise1, noise2), 0.0, 1.0);
}

// Blur calculations
vec3 blur(vec2 coords, float blurAmount)
{
	vec2 noise = rand(coords);
	float tolerance = 0.2; // From uniform?
	float vignette_size = 0.5; // From uniform?
	vec2 powers = pow(abs(vec2(coords.x - 0.5, coords.y - 0.5)), vec2(2.0));
	float radiusSqrd = vignette_size * vignette_size;
	float gradient = smoothstep(radiusSqrd - tolerance, radiusSqrd + tolerance, powers.x + powers.y);

	// Exit on min gradient
	if (gradient < 0.07) 
	{
		return texture(scene_texture, coords).rgb;
	}
	// Output color
	vec4 color = vec4(0.0);

	// Blur and vignette
	float X1 = coords.x + blurAmount * noise.x * 0.004 * gradient;
	float Y1 = coords.y + blurAmount * noise.y * 0.004 * gradient;
	float X2 = coords.x - blurAmount * noise.x * 0.004 * gradient;
	float Y2 = coords.y - blurAmount * noise.y * 0.004 * gradient;

	float invX1 = coords.x + blurAmount * ((1.0 - noise.x) * 0.004) * (gradient * 0.5);
	float invY1 = coords.y + blurAmount * ((1.0 - noise.y) * 0.004) * (gradient * 0.5);
	float invX2 = coords.x - blurAmount * ((1.0 - noise.x) * 0.004) * (gradient * 0.5);
	float invY2 = coords.y - blurAmount * ((1.0 - noise.y) * 0.004) * (gradient * 0.5);

	color += texture(scene_texture, vec2(X1, Y1)) * 0.1;
	color += texture(scene_texture, vec2(X2, Y2)) * 0.1;
	color += texture(scene_texture, vec2(X1, Y2)) * 0.1;
	color += texture(scene_texture, vec2(X2, Y1)) * 0.1;

	color += texture(scene_texture, vec2(invX1, invY1)) * 0.15;
	color += texture(scene_texture, vec2(invX2, invY2)) * 0.15;
	color += texture(scene_texture, vec2(invX1, invY2)) * 0.15;
	color += texture(scene_texture, vec2(invX2, invY1)) * 0.15;

	return color.rgb;
}

void main(void)
{
	// Calculate screen position of the fragment [0-1]
	vec2 screen_coords = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
	// Calculate distortion
	color = blur(screen_coords, blurIntensity);
}
