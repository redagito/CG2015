#pragma once

#include <cstdint>

typedef int64_t ResourceId;
static const ResourceId invalidResource = -1;

enum class AlphaMode { Opaque, Mask, Blend };
struct SMaterialSettings
{
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    float opacity = 1.f;
    bool srgb = true;
    bool background = false;
};

/**
 * \brief Resource types.
 */
enum class EResourceType
{
    Mesh,
    Image,
    String,
    Shader,
    Material,
	Model
};

/**
* \brief Primitive type for vertex data.
* NOTE GL_QUAD is deprecated.
*/
enum class EPrimitiveType
{
    Point = 1,
    Line = 2,
    Triangle = 3,
    Invalid
};

/**
* \brief Color format for texture data.
*/
enum class EColorFormat
{
    GreyScale8 = 1,
    RGB24 = 3,
    RGBA32 = 4,
    Invalid
};

/**
 * \brief Possible listener events.
 */
enum class EListenerEvent
{
    Create, /**< Specified resource was created. */
    Change, /**< Specified resource was changed. */
    Delete  /**< Specified resource was deleted. */
};
