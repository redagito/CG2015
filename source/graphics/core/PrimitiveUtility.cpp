#include "PrimitiveUtility.h"

#include "log/Log.h"

unsigned int getSize(EPrimitiveType type)
{
	switch (type)
	{
	case EPrimitiveType::Point:
		return 1;
	case EPrimitiveType::Line:
		return 2;
	case EPrimitiveType::Triangle:
		return 3;
	}
	LOG_CRITICAL("Invalid or unknown primitive type encountered.");
	return 0;
}

GLenum toGLPrimitive(EPrimitiveType type)
{
	switch (type)
	{
	case EPrimitiveType::Point:
		return GL_POINTS;
	case EPrimitiveType::Line:
		return GL_LINE;
	case EPrimitiveType::Triangle:
		return GL_TRIANGLES;
	}
	
	LOG_ERROR("Invalid or unknown primitive type, default triangle type used.");
	return GL_TRIANGLES;
}