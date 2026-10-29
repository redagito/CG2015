#version 460 core
#include "emissive_accents.glsl"

in vec2 uv;
smooth in vec3 vertexWorldSpace;
smooth in vec3 normalVectorWorldSpace;

// Material textures
uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;
uniform sampler2D specular_texture;
uniform sampler2D glow_texture;

// Actually not needed here
uniform sampler2D alpha_texture;
uniform bool normal_mapping_enabled = true;
uniform int alpha_mode=0;
uniform float alpha_cutoff=0.5;
uniform float opacity=1.0;

// Diffuse color and glow value
layout(location = 0) out vec4 diffuse_glow;
// Normals and specular value
layout(location = 1) out vec4 normal_specular;

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv )
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float invmax = inversesqrt(max(max(dot(T,T), dot(B,B)), 0.00000001));
    return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    vec3 map = texture( normal_texture, texcoord ).xyz;
    map = map * 2.0 - 1.0;
    mat3 TBN = cotangent_frame(normalize(N), V, texcoord);
    vec3 perturbed = TBN * map;
    return dot(perturbed, perturbed) > 0.00000001 ? normalize(perturbed) : normalize(N);
}

void main(void)
{
    if(alpha_mode==1 && texture(alpha_texture,uv).r*opacity<alpha_cutoff) discard;
	vec3 color = texture(diffuse_texture, uv).rgb;
	vec3 normal = texture(normal_texture, uv).rgb;

	float specular = texture(specular_texture, uv).r;
	float glow = texture(glow_texture, uv).r;
    applyEmissiveAccents(color, glow);
	//float alpha = texture(alpha_texture, uv).r;

	// Write diffuse map with glow
	diffuse_glow.rgb = color;
	diffuse_glow.a = glow;

	// normal.rgb = textureNormal;
    normal_specular.rgb = normal_mapping_enabled
        ? perturb_normal(normalVectorWorldSpace, vertexWorldSpace, uv)
        : normalize(normalVectorWorldSpace);
	normal_specular.a = specular;
}
