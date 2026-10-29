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
uniform mat4 translation;
uniform mat4 scale;

// View and projection matrices
uniform mat4 view;
uniform mat4 projection;

out vec4 world_pos;

out vec2 uv;
void main(void)
{
    mat4 objectModel = instanced ? instanceModels[gl_InstanceID] : model;
    // Calculate vertex position in camera space
    uv=vertexUV;
    gl_Position = projection * view * objectModel * vec4(vertexPositionModelSpace, 1.f);
    world_pos = objectModel * vec4(vertexPositionModelSpace, 1.0f);
}
