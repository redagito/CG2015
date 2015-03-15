#pragma once

#include "CoreConfig.h"

/**
* \brief Primitive types supported.
*/
enum class EPrimitiveType
{
	Point,
	Line,
	Triangle,
	Invalid
};

unsigned int getSize(EPrimitiveType type);

GLenum toGLPrimitive(EPrimitiveType type);