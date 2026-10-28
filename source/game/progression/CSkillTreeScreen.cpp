#include "CSkillTreeScreen.h"
#include "CPlayerProgression.h"
#include "input/IInputProvider.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <format>
CSkillTreeScreen::CSkillTreeScreen(CPlayerProgression &progression, bool victory)
    : m_progression(progression), m_victory(victory)
{
}
glm::vec2 CSkillTreeScreen::nodeAnchor(unsigned node)
{
    return {.18f + .32f * (node / 2), .37f + .23f * (node % 2)};
}
bool CSkillTreeScreen::contains(glm::vec2 anchor, glm::vec2 size, glm::vec2 cursor,
                                glm::vec2 viewport)
{
    const float scale = std::max(.01f, std::min(viewport.x / 1280.f, viewport.y / 720.f));
    return glm::all(glm::lessThanEqual(glm::abs(cursor - anchor * viewport), size * scale * .5f));
}
void CSkillTreeScreen::onEnter(IInputProvider &input)
{
    m_selected = 0;
    m_clickHeld = input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    m_enterHeld = input.isKeyPressed(GLFW_KEY_ENTER);
    m_tabHeld = input.isKeyPressed(GLFW_KEY_TAB);
    m_retryHeld = input.isKeyPressed(GLFW_KEY_R);
}
bool CSkillTreeScreen::update(IInputProvider &input)
{
    const bool click = input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool enter = input.isKeyPressed(GLFW_KEY_ENTER), tab = input.isKeyPressed(GLFW_KEY_TAB),
               retry = input.isKeyPressed(GLFW_KEY_R);
    if (tab && !m_tabHeld)
        m_selected = (m_selected + 1) % 7;
    bool restart = retry && !m_retryHeld;
    if (click && !m_clickHeld)
    {
        for (unsigned i = 0; i < 6; ++i)
            if (contains(nodeAnchor(i), {300, 116}, input.cursorPosition(), input.viewportSize()))
            {
                m_selected = i;
                m_progression.buy(static_cast<SkillId>(i));
            }
        restart |= contains({.5f, .82f}, {340, 64}, input.cursorPosition(), input.viewportSize());
    }
    if (enter && !m_enterHeld)
    {
        if (m_selected == 6)
            restart = true;
        else
            m_progression.buy(static_cast<SkillId>(m_selected));
    }
    m_clickHeld = click;
    m_enterHeld = enter;
    m_tabHeld = tab;
    m_retryHeld = retry;
    return !restart;
}
UiDocument CSkillTreeScreen::build(glm::vec2 viewport, glm::vec2 cursor) const
{
    UiDocument ui;
    const float scale = std::max(.01f, std::min(viewport.x / 1280.f, viewport.y / 720.f));
    const glm::vec4 white(.95f, .97f, 1, 1), cyan(.35f, .85f, 1, 1), green(.4f, 1, .65f, 1),
        muted(.5f, .57f, .65f, 1);
    ui.rectangles.push_back({{.5f, .5f}, viewport / scale, {.012f, .02f, .035f, .97f}});
    ui.texts.push_back({m_victory ? "YOU WON" : "YOU LOST", {.5f, .065f}, 42, white, 5, .9f});
    ui.texts.push_back({std::format("SKILL POINTS {}    /    TOTAL KILLS {}",
                                    m_progression.points(), m_progression.earned()),
                        {.5f, .145f},
                        24,
                        cyan,
                        1,
                        .9f});
    const char *branches[] = {"WEAPONS", "HANDLING", "ENGINES"};
    for (unsigned i = 0; i < 3; ++i)
    {
        const float x = .18f + .32f * i;
        ui.texts.push_back({branches[i], {x, .235f}, 23, cyan, 2, .29f});
        const auto color = m_progression.rank(static_cast<SkillId>(i * 2)) > 0 ? green : muted;
        ui.rectangles.push_back({{x, .485f}, {3, viewport.y * .23f / scale}, color});
        ui.texts.push_back({"v", {x, .485f}, 20, color, 0, .1f});
    }
    // Keep glyphs visible in small windows while constraining text to its card.
    const float textScale = std::max(1.f, .38f / scale);
    const float cardTextWidth = 280 * scale / std::max(1.f, viewport.x);
    for (const auto &skill : CPlayerProgression::skills())
    {
        const auto index = static_cast<unsigned>(skill.id);
        const auto anchor = nodeAnchor(index);
        const bool hovered = contains(anchor, {300, 116}, cursor, viewport);
        const bool available = m_progression.canBuy(skill.id),
                   unlocked = m_progression.unlocked(skill.id);
        const bool maxed = m_progression.rank(skill.id) == skill.maxRank;
        const auto accent = maxed ? green : available ? cyan : muted;
        ui.rectangles.push_back(
            {anchor,
             {306, 122},
             m_selected == index || hovered ? accent : glm::vec4(.09f, .14f, .2f, 1)});
        ui.rectangles.push_back({anchor, {300, 116}, {.025f, .04f, .065f, 1}});
        ui.texts.push_back(
            {std::format("{}  {} / {}", skill.name, m_progression.rank(skill.id), skill.maxRank),
             anchor + glm::vec2(0, -32 * scale / viewport.y), 20 * textScale, accent, 1,
             cardTextWidth});
        ui.texts.push_back({std::string(skill.description),
                            anchor + glm::vec2(0, 1 * scale / viewport.y), 16 * textScale, white, 0,
                            cardTextWidth});
        const auto label = maxed ? std::string("MAX RANK")
                           : !unlocked
                               ? std::string("REQUIRES NODE ABOVE")
                               : std::format("{} POINT{} / {}", m_progression.cost(skill.id),
                                             m_progression.cost(skill.id) == 1 ? "" : "S",
                                             available ? "UPGRADE" : "NEED POINTS");
        ui.texts.push_back({label, anchor + glm::vec2(0, 34 * scale / viewport.y), 15 * textScale,
                            accent, 0, cardTextWidth});
    }
    const bool retryHover = contains({.5f, .82f}, {340, 64}, cursor, viewport);
    ui.rectangles.push_back({{.5f, .82f}, {346, 70}, retryHover || m_selected == 6 ? white : cyan});
    ui.rectangles.push_back({{.5f, .82f}, {340, 64}, {.04f, .16f, .22f, 1}});
    ui.texts.push_back({"RETRY RUN", {.5f, .82f}, 27, white, 3, .8f});
    ui.texts.push_back(
        {"CLICK TO UPGRADE / TAB SELECT / ENTER BUY / R RETRY", {.5f, .925f}, 16, muted, 0, .94f});
    ui.texts.push_back(
        {"POINTS AND SKILLS LAST UNTIL YOU CLOSE THE GAME", {.5f, .965f}, 14, muted, 0, .94f});
    return ui;
}
