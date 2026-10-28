#include "CRemoveOnDeathController.h"
#include "game/CGameObject.h"
CRemoveOnDeathController::CRemoveOnDeathController(std::function<void()> onDefeat)
    : m_onDefeat(std::move(onDefeat))
{
}
CRemoveOnDeathController::~CRemoveOnDeathController() = default;
void CRemoveOnDeathController::attach(CGameObject *object)
{
    m_object = object;
}
void CRemoveOnDeathController::detach()
{
    m_object = nullptr;
}
void CRemoveOnDeathController::setActive(bool active)
{
    m_active = active;
}
void CRemoveOnDeathController::receiveMessage(Message)
{
}
void CRemoveOnDeathController::update(float)
{
    if (m_active && m_object && m_object->isDead() && !m_object->isDeleteRequested())
    {
        if (m_onDefeat)
            m_onDefeat();
        m_object->markDeleted();
    }
}
