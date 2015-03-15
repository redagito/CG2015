#pragma once

#include <string>

#include "CoreConfig.h"

/**
* \brief Defines the application to shader interface.
*/

// Vertex array data locations
const GLuint vertexDataShaderLocation = 0;
const GLuint normalDataShaderLocation = 1;
const GLuint uvDataShaderLocation = 2;

/**
* \brief Common uniform names used by multiple shaders with defined symantics.
*/

// Screen parameters
const std::string screenSizeUniformName = "screen_size"; /**< Respresents screen width and height as a 2d vector. */

// Camera parameters
const std::string cameraPositionUniformName = "camera_position"; /**< Represents camera position in world coordinates as a 3d vector. */
const std::string cameraDirectionUniformName = "camera_direction"; /**< Represents camera view direction in world coordinates as a 3d vector. */
const std::string cameraZNearUniformName = "camera_z_near"; /**< Represents near camera plane as a float. */
const std::string cameraZFarUniformName = "camera_z_far"; /**< Represents far camera plane as a float. */
const std::string cameraFOVUniformName = "camera_fov"; /**< Represents camera field of view in degrees. */

// Transformation matrix parameters
const std::string rotationMatrixUniformName = "rotation";
const std::string inverseRotationMatrixUniformName = "inverse_rotation";
const std::string translationMatrixUniformName = "translation";
const std::string inverseTranslationMatrixUniformName = "inverse_translation";
const std::string scaleMatrixUniformName = "scale";
const std::string inverseScaleMatrixUniformName = "inverse_scale";

const std::string modelMatrixUniformName = "model";
const std::string inverseModelMatrixUniformName = "inverse_model";
const std::string viewMatrixUniformName = "view";
const std::string inverseViewMatrixUniformName = "inverse_view";
const std::string projectionMatrixUniformName = "projection";
const std::string inverseProjectionMatrixUniformName = "inverse_projection";

const std::string viewProjectionMatrixUniformName = "view_projection";
const std::string inverseViewProjectionMatrixUniformName = "inverse_view_projection";
const std::string modelViewProjectionMatrixUniformName = "model_view_projection";
const std::string inverseModelViewProjectionMatrixUniformName = "model_view_projection";

// Texture sampler uniform names
const std::string diffuseTextureUniformName = "diffuse_texture";
const std::string normalTextureUniformName = "normal_texture";
const std::string specularTextureUniformName = "specular_texture";
const std::string glowTextureUniformName = "glow_texture";
const std::string alphaTextureUniformName = "alpha_texture";
const std::string depthTextureUniformName = "depth_texture";
const std::string shadowMapTextureUniformName = "shadow_map_texture";
const std::string shadowCubeTextureUniformName = "shadow_cube_texture";
const std::string sceneTextureUniformName = "scene_texture"; /**< Stores currently rendered scene, */