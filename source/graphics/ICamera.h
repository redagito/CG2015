#pragma once
#include "SFeatureInfo.h"

#include <glm/glm.hpp>

/**
 * \brief Camera interface class.
 */
class ICamera
{
  public:
    virtual ~ICamera();

    /**
     * \brief Returns view matrix.
     */
    virtual const glm::mat4 &getView() const = 0;

    /**
     * \brief Returns projection matrix.
     */
    virtual const glm::mat4 &getProjection() const = 0;

    virtual const glm::vec3 &getPosition() const = 0;

    // TODO extract functionality to a better place
    virtual SFeatureInfo &getFeatureInfoForWrite()
    {
        return m_info;
    } // TODO Rename
    virtual const SFeatureInfo &getFeatureInfo() const
    {
        return m_info;
    }

  private:
    SFeatureInfo m_info;
};
