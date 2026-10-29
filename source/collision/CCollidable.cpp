#include "CCollidable.h"
#include "CCollisionSystem.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

CCollidable::CCollidable(unsigned int group, const CAABBox &box, std::uint64_t id,
                         CCollisionSystem *owner)
    : m_id(id), m_owner(owner), m_groupId(group), m_damageDealt(0.f), m_damageReceived(0.f),
      m_box(box), m_boxTransformed(box), m_deleteMe(false), m_collidable(true)
{
    m_previousBox = m_boxTransformed;
    return;
}

float CCollidable::getDamage() const
{
    return m_damageDealt;
}

// Returns accumuated damage and resets damage counter
float CCollidable::getDamageReceived()
{
    float temp = m_damageReceived;
    m_damageReceived = 0.f;
    return temp;
}

// Returns group id
unsigned int CCollidable::getGroupId() const
{
    return m_groupId;
}

// Add damage to the internal counter
float CCollidable::receiveDamage(float damage)
{
    return receiveDamage(damage, m_boxTransformed.getMid() + glm::vec3(0, 0, -1));
}
float CCollidable::receiveDamage(float damage, glm::vec3 source, DamageKind kind)
{
    if (!std::isfinite(damage) || damage <= 0)
        return 0;
    const float before = remainingHealthBudget();
    m_damageReceived += damage;
    m_damageSource = source;
    const float after = remainingHealthBudget();
    // Later pellets cannot claim overkill damage or replace the original lethal hit's origin.
    if (before > 0 && after <= 0)
        m_fatalDamageKind = kind;
    return before - after;
}
void CCollidable::setHealthBudget(float hull, float shield, float armor)
{
    m_hullBudget = std::max(0.f, hull);
    m_shieldBudget = std::max(0.f, shield);
    m_armor = std::clamp(armor, 0.f, .8f);
    if (hull > 0 && m_damageReceived == 0)
        m_fatalDamageKind = DamageKind::Direct;
}
float CCollidable::remainingHealthBudget() const
{
    if (m_hullBudget < 0)
        return 0; // Receivers without health never grant lifesteal.
    return std::max(0.f, m_hullBudget -
                             std::max(0.f, m_damageReceived * (1 - m_armor) - m_shieldBudget));
}
DamageKind CCollidable::fatalDamageKind() const
{
    return m_fatalDamageKind;
}

glm::vec3 CCollidable::damageSource() const
{
    return m_damageSource;
}

// Sets the damage dealt by this entity
void CCollidable::setDamage(float damage)
{
    m_damageDealt = damage;
}

const CAABBox &CCollidable::getAABBox() const
{
    return m_boxTransformed;
}

void CCollidable::setScale(const glm::vec3 &scale)
{
    setTransform(m_translation, m_orientation, scale);
}
void CCollidable::setTranslation(const glm::vec3 &translation)
{
    setTransform(translation, m_orientation, m_scale);
}
void CCollidable::setTransform(glm::vec3 translation, glm::quat orientation, glm::vec3 scale)
{
    if (m_owner && m_collidable)
        m_owner->invalidateQueries();
    m_translation = translation;
    m_orientation = orientation;
    m_scale = scale;
    const auto rotation = glm::mat3_cast(orientation);
    const glm::mat3 absolute(glm::abs(rotation[0]), glm::abs(rotation[1]), glm::abs(rotation[2]));
    // Transform the original center/extents; using scale itself as extents shrinks every ship.
    m_boxTransformed.setMid(translation + rotation * (m_box.getMid() * scale));
    m_boxTransformed.setHalfWidths(absolute * (m_box.getHalfWidths() * glm::abs(scale)));
    if (!m_trackingMotion)
        m_previousBox = m_boxTransformed;
}

// Sets collidable state
void CCollidable::setCollidable(bool state)
{
    if (m_owner)
        m_owner->invalidateQueries();
    m_collidable = state;
}

// Returns collidable state
bool CCollidable::isCollidable() const
{
    return m_collidable;
}

bool CCollidable::deleteRequested() const
{
    return m_deleteMe;
}

void CCollidable::markDeleted()
{
    m_deleteMe = true;
}
void CCollidable::capturePreviousBounds()
{
    m_previousBox = m_boxTransformed;
    m_trackingMotion = true;
}
const CAABBox &CCollidable::previousBounds() const
{
    return m_previousBox;
}

std::uint64_t CCollidable::id() const
{
    return m_id;
}
