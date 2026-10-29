#version 460 core

// Vertex data streams
layout (location = 0) in vec3 vertexPositionModelSpace;
layout (location = 1) in vec3 vertexNormalModelSpace;
layout (location = 2) in vec2 vertexUV;

// Transformation matrices
uniform mat4 model;
uniform bool instanced = false;
layout(std430,binding=7) readonly buffer InstanceTransforms { mat4 instanceModels[]; };
uniform mat4 rotation;

// View and projection matrices
uniform mat4 view;
uniform mat4 projection;

// Texture coordinate
out vec2 uv;
out vec3 accentPositionModelSpace, accentNormalModelSpace;
// Normal vector
smooth out vec3 normalVectorWorldSpace;

smooth out vec3 vertexWorldSpace;

void main(void)
{
    mat4 objectModel = instanced ? instanceModels[gl_InstanceID] : model;
	// Calculate vertex position in camera space
	gl_Position = projection * view * objectModel * vec4(vertexPositionModelSpace, 1.f);

	// Forward texture coordinates
	uv = vertexUV;
    accentPositionModelSpace = vertexPositionModelSpace;
    accentNormalModelSpace = vertexNormalModelSpace;

    vertexWorldSpace = (objectModel * vec4(vertexPositionModelSpace, 1.0)).xyz;
    normalVectorWorldSpace = normalize(transpose(inverse(mat3(objectModel))) * vertexNormalModelSpace);
}
