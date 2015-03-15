#pragma once

#include "graphics/api/PrimitiveType.h"
#include "CoreConfig.h"

unsigned int getSize(EPrimitiveType type);

GLenum toGLPrimitive(EPrimitiveType type);