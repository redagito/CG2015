#pragma once

#include "CAABBox.h"
#include "DamageKind.h"
#include <cstdint>
class CCollisionSystem;

#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>

/**
 * \brief Represents a collision entity in the collision system.
 *
 * A collidable entity deals a certain amount of damage to the other entity.
 * This is used for modeling projectile with ship collision.
 */
class CCollidable
{
  public:
    /**
     * \brief The entity is created with its group id and the untransformed bounding volume
     */
    CCollidable(unsigned int groupId, const CAABBox &box, std::uint64_t id = 0,
                CCollisionSystem *owner = nullptr);
    std::uint64_t id() const;

    /**
     * \brief Returns the group id
     */
    unsigned int getGroupId() const;

    /**
     * \brief Sets the amount of damage the entity deals on collision.
     */
    void setDamage(float damage);

    /**
     * \brief Returns the amount of damage the entity deals.
     */
    float getDamage() const;

    /**
     * \brief Returns the amount of damage the entity received since the last call.
     *
     * Calling this function resets the damage counter on the entity.
     */
    float getDamageReceived();

    /**
     * \brief Queues damage and returns actual hull damage after armor, shields and pending hits.
     */
    float receiveDamage(float damage);
    float receiveDamage(float damage, glm::vec3 source, DamageKind kind = DamageKind::Direct);
    // Health controllers publish a snapshot; pending hits are subtracted before crediting damage.
    void setHealthBudget(float hull, float shield, float armor);
    float remainingHealthBudget() const;
    DamageKind fatalDamageKind() const;
    glm::vec3 damageSource() const;

    /**
     * \brief Returns the bounding box.
     */
    const CAABBox &getAABBox() const;

    /**
     * \brief Sets the translation matrix for the entity
     */
    void setTranslation(const glm::vec3 &translation);
    void capturePreviousBounds();
    const CAABBox &previousBounds() const;
    void setTransform(glm::vec3 translation, glm::quat orientation, glm::vec3 scale);

    /**
     * \brief Sets the scale matrix
     */
    void setScale(const glm::vec3 &scale);

    /**
     * \brief Sets the collision status.
     *
     * If this is set to false, the entity will get ignored while collision testing
     */
    void setCollidable(bool);

    /**
     * \brief Returns whether or not the entity should be used for collision testinG
     */
    bool isCollidable() const;

    /**
     * \brief Requests deletion of the object
     */
    void markDeleted();

    /**
     * \brief Checks for deletion request
     */
    bool deleteRequested() const;

  protected:
    friend class CCollisionSystem;
    mutable std::uint64_t m_queryStamp = 0;
    std::uint64_t m_id = 0;
    CCollisionSystem *m_owner = nullptr;
    glm::vec3 m_translation{0}, m_scale{1};
    glm::quat m_orientation{1, 0, 0, 0};
    CAABBox
        m_boxTransformed; /**< Transformed collision volume, recalculated on setTranslation call. */
    CAABBox m_previousBox;
    bool m_trackingMotion = false;
    CAABBox m_box;              /**< The untransformed collision volume. */
    unsigned int m_groupId = 0; /**< Collision group id. */

    glm::vec3 m_damageSource{0};
    float m_hullBudget = -1, m_shieldBudget = 0, m_armor = 0;
    DamageKind m_fatalDamageKind = DamageKind::Direct;
    float m_damageReceived =
        0.f; /**< Accumulated damage that was received since the last call to getDamageReceived. */
    float m_damageDealt =
        0.f; /**< Amount of damage this entity deals on collision to the other entity. */

    bool m_deleteMe = false;  /**< Stores deletion state. */
    bool m_collidable = true; /**< Stores whether or not the collision entity is active. */
};