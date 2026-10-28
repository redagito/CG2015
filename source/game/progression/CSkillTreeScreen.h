#pragma once
#include "graphics/ui/UiDocument.h"
#include <glm/glm.hpp>
class CPlayerProgression;
class IInputProvider;
class CSkillTreeScreen
{
  public:
    explicit CSkillTreeScreen(CPlayerProgression &progression, bool victory = false);
    void onEnter(IInputProvider &input);
    bool update(IInputProvider &input); // False requests a new run.
    UiDocument build(glm::vec2 viewport, glm::vec2 cursor) const;

  private:
    static glm::vec2 nodeAnchor(unsigned node);
    static bool contains(glm::vec2 anchor, glm::vec2 size, glm::vec2 cursor, glm::vec2 viewport);
    CPlayerProgression &m_progression;
    bool m_victory;
    unsigned m_selected = 0;
    bool m_clickHeld = false, m_enterHeld = false, m_tabHeld = false, m_retryHeld = false;
};
