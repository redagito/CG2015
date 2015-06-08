#version 330 core


uniform sampler2D scene_texture;
uniform sampler1D lenscolor_texture;

uniform float screen_width;
uniform float screen_height;

const int MAX_SAMPLES = 64;
uniform int uSamples = 8;

uniform float uDispersal = 0.25;
uniform float uHaloWidth = 1.0;
uniform float uDistortion = 1.0;

noperspective in vec2 vTexcoord;

layout(location=0) out vec4 color;

/*----------------------------------------------------------------------------*/
vec4 textureDistorted(
	in sampler2D tex, 
	in vec2 texcoord, 
	in vec2 direction,
	in vec3 distortion 
) {
	return vec4(
		texture(tex, texcoord + direction * distortion.r).r,
		texture(tex, texcoord + direction * distortion.g).g,
		texture(tex, texcoord + direction * distortion.b).b,
		1.0
	);
}

/*----------------------------------------------------------------------------*/

// Lens flare pass
// References
// http://john-chapman-graphics.blogspot.de/2013/02/pseudo-lens-flare.html
vec4 lensflare(vec3 texeture) {
	vec2 texcoord = -vTexcoord + vec2(1.0); // flip texcoordoords
	vec2 texelSize = 1.0 / vec2(textureSize(scene_texture, 0));
	
	vec2 ghostVec = (vec2(0.5) - texcoord) * uDispersal;
	vec2 haloVec = normalize(ghostVec) * uHaloWidth;
	
	vec3 distortion = vec3(-texelSize.x * uDistortion, 0.0, texelSize.x * uDistortion);

// sample ghosts:
	vec4 result = vec4(0.0);
	for (int i = 0; i < uSamples; ++i) {
		vec2 offset = fract(texcoord + ghostVec * float(i));
		
		float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
		weight = pow(1.0 - weight, 10.0);
	
		/*result += textureDistorted(
			scene_texture,
			offset,
			normalize(ghostVec),
			distortion
		) * weight;*/
	}
	
	result *= texture(lenscolor_texture, length(vec2(0.5) - texcoord) / length(vec2(0.5)));

//	sample halo:
	float weight = length(vec2(0.5) - fract(texcoord + haloVec)) / length(vec2(0.5));
	weight = pow(1.0 - weight, 10.0);
	/*result += textureDistorted(
		scene_texture,
		fract(texcoord + haloVec),
		normalize(ghostVec),
		distortion
	) * weight;*/
	
	return result;
}

void main() {
	// Calculate screen position of the fragment [0-1]
	vec2 screen_coords = vec2(gl_FragCoord.x / screen_width, gl_FragCoord.y / screen_height);
	
	// Get scene texel
	vec3 texel = texture(scene_texture, screen_coords).rgb;

	// Apply advanced bloom
	color = lensflare(texel);
}