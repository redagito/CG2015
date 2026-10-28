#include "CTargetingSystem.h"
#include "ProjectileBallistics.h"
#include "input/IInputProvider.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/ext.hpp>

CTargetingSystem::CTargetingSystem(const TargetingTuning &tuning) : m_tuning(tuning)
{
}
const std::optional<TargetingSolution> &CTargetingSystem::solution() const
{
    return m_solution;
}
void CTargetingSystem::update(const std::vector<TargetSnapshot> &targets, glm::vec3 position,
                              glm::vec3 forward, const WeaponAim &aim, IInputProvider &input)
{
    const bool tab = input.isKeyPressed(GLFW_KEY_TAB), select = input.isKeyPressed(GLFW_KEY_T),
               clear = input.isKeyPressed(GLFW_KEY_X);
    std::vector<const TargetSnapshot *> nearby;
    for (const auto &target : targets)
        if (glm::distance(position, target.position) <= m_tuning.range)
            nearby.push_back(&target);
    std::sort(nearby.begin(), nearby.end(),
              [](const auto *a, const auto *b) { return a->id < b->id; });
    const auto selected = std::find_if(nearby.begin(), nearby.end(),
                                       [&](const auto *t) { return t->id == m_selected; });
    if (selected == nearby.end())
        m_selected = noGameObject;
    if (clear && !m_clearDown)
    {
        m_selected = noGameObject;
        m_initialAcquisition = false;
    }
    else if (tab && !m_tabDown && !nearby.empty())
    {
        const auto next = selected == nearby.end() || std::next(selected) == nearby.end()
                              ? nearby.begin()
                              : std::next(selected);
        m_selected = (*next)->id;
        m_initialAcquisition = false;
    }
    else if ((select && !m_selectDown) || m_initialAcquisition)
    {
        float best = std::cos(glm::radians(m_tuning.acquisitionAngle));
        GameObjectId candidate = noGameObject;
        for (const auto *target : nearby)
        {
            const auto delta = target->position - position;
            if (glm::dot(delta, delta) < 1e-8f)
                continue;
            const float score = glm::dot(glm::normalize(delta), forward);
            if (score > best)
            {
                best = score;
                candidate = target->id;
            }
        }
        m_selected = candidate;
        if (candidate)
            m_initialAcquisition = false;
    }
    m_tabDown = tab;
    m_selectDown = select;
    m_clearDown = clear;
    m_solution.reset();
    for (const auto *target : nearby)
        if (target->id == m_selected)
        {
            m_solution = TargetingSolution{
                *target, ProjectileBallistics::intercept(aim, target->position, target->velocity),
                glm::distance(position, target->position)};
            break;
        }
}
