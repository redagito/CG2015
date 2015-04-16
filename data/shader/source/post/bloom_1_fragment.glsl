#version 400

uniform sampler2D scene_texture;
uniform float screen_width;
uniform float screen_height;

layout (location = 0) out vec3 color;

vec3 simpleBloom(vec3 texel)
{
	// Desaturate
	float grey = max(texel.r, max(texel.g, texel.b));

	// Threshold
	return vec3(clamp(((grey) - 0.3), 0.0, 0.3));
}

vec3 advancedBloom(vec3 texel)
{
	// Desaturate
	vec3 lumCoeff = vec3(0.2125, 0.7154, 0.0721);

	vec3 avgLumin = vec3(0.5f, 0.5f, 0.5f);
	float intensity = dot(texel, lumCoeff);
	return vec3(intensity);
}

// Writes bloom texture
void main()
{
	// Calculate screen position of the fragment [0-1]
	vec2 screen_coords = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
	
	// Get scene texel
	vec3 texel = texture(scene_texture, screen_coords).rgb;

	// Apply simple bloom
	color = simpleBloom(texel);

	// Apply advanced bloom
	color = advancedBloom(texel);
}
