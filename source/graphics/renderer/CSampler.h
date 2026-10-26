#pragma once

#include "core/RendererCoreConfig.h"

    /**
* \brief Opengl sampler wrapper.
*/
class CSampler
{
public:

    CSampler(const CSampler&) = delete;
    CSampler& operator=(const CSampler&) = delete;
    CSampler();
	~CSampler();

	/**
	* \brief Bind to texture unit.
	*/
	void bind(GLuint unit);

	void setMinFilter(GLint filter);
	void setMagFilter(GLint filter);

private:
	GLuint m_samplerId = 0;
};
