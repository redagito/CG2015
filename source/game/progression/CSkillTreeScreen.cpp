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
    m_branch = 0;
    for (unsigned page = 0; page < 4; ++page)
        m_pageHeld[page] = input.isKeyPressed(GLFW_KEY_1 + page);
    m_clickHeld = input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    m_enterHeld = input.isKeyPressed(GLFW_KEY_ENTER);
    m_tabHeld = input.isKeyPressed(GLFW_KEY_TAB);
    m_retryHeld = input.isKeyPressed(GLFW_KEY_R);
}
std::array<const SkillDefinition *, 6> CSkillTreeScreen::visibleSkills() const
{
    std::array<const SkillDefinition *, 6> result{};
    for (const auto &skill : CPlayerProgression::skills())
        if (unsigned(skill.branch) == m_branch)
            result[skill.slot] = &skill;
    return result;
}
bool CSkillTreeScreen::update(IInputProvider &input)
{
    const bool click = input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT),
               enter = input.isKeyPressed(GLFW_KEY_ENTER), tab = input.isKeyPressed(GLFW_KEY_TAB),
               retry = input.isKeyPressed(GLFW_KEY_R);
    for (unsigned page = 0; page < 4; ++page)
    {
        const bool pressed = input.isKeyPressed(GLFW_KEY_1 + page);
        if (pressed && !m_pageHeld[page])
        {
            m_branch = page;
            m_selected = 0;
        }
        m_pageHeld[page] = pressed;
    }
    if (tab && !m_tabHeld)
        m_selected = (m_selected + 1) % 7;
    bool restart = retry && !m_retryHeld;
    if (click && !m_clickHeld)
    {
        for (unsigned page = 0; page < 4; ++page)
            if (contains({.14f + .24f * page, .235f}, {260, 42}, input.cursorPosition(),
                         input.viewportSize()))
            {
                m_branch = page;
                m_selected = 0;
            }
        const auto visible = visibleSkills();
        for (unsigned slot = 0; slot < 6; ++slot)
            if (contains(nodeAnchor(slot), {300, 116}, input.cursorPosition(),
                         input.viewportSize()))
            {
                m_selected = slot;
                m_progression.buy(visible[slot]->id);
            }
        restart |= contains({.5f, .82f}, {340, 64}, input.cursorPosition(), input.viewportSize());
    }
    if (enter && !m_enterHeld)
    {
        if (m_selected == 6)
            restart = true;
        else
            m_progression.buy(visibleSkills()[m_selected]->id);
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
    const char *branches[] = {"GUNNERY", "ORDNANCE", "DEFENSE", "FLIGHT"};
    for (unsigned page = 0; page < 4; ++page)
    {
        const auto anchor = glm::vec2(.14f + .24f * page, .235f);
        const bool hover = contains(anchor, {260, 42}, cursor, viewport);
        ui.rectangles.push_back(
            {anchor, {264, 46}, page == m_branch || hover ? cyan : glm::vec4(.08f, .12f, .18f, 1)});
        ui.rectangles.push_back({anchor, {260, 42}, {.025f, .04f, .065f, 1}});
        ui.texts.push_back({std::format("{} / {}", page + 1, branches[page]), anchor, 20,
                            page == m_branch ? white : muted, 1, .22f});
    }
    const auto visible = visibleSkills();
    for (const auto *skill : visible)
    {
        if (skill->prerequisite == SkillId::Count)
            continue;
        const auto found = std::find_if(visible.begin(), visible.end(), [&](const auto *node)
                                        { return node->id == skill->prerequisite; });
        if (found == visible.end())
            continue;
        const auto from = nodeAnchor((*found)->slot), to = nodeAnchor(skill->slot);
        const auto color = m_progression.unlocked(skill->id) ? green : muted;
        ui.rectangles.push_back({{(from.x + to.x) * .5f, from.y},
                                 {std::abs(from.x - to.x) * viewport.x / scale + 2, 2},
                                 color});
        ui.rectangles.push_back({{to.x, (from.y + to.y) * .5f},
                                 {2, std::abs(from.y - to.y) * viewport.y / scale + 2},
                                 color});
    }
    // Keep glyphs visible in small windows while constraining text to its card.
    const float textScale = std::max(1.f, .38f / scale);
    const float cardTextWidth = 280 * scale / std::max(1.f, viewport.x);
    for (const auto *node : visible)
    {
        const auto &skill = *node;
        const auto index = skill.slot;
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
             anchor + glm::vec2(0, -40 * scale / viewport.y), 20 * textScale, accent, 1,
             cardTextWidth});
        const std::string description(skill.description);
        if (description.size() > 24)
        {
            const auto split = description.rfind(' ', description.size() / 2);
            ui.texts.push_back({description.substr(0, split),
                                anchor + glm::vec2(0, -12 * scale / viewport.y), 16 * textScale,
                                white, 0, cardTextWidth});
            ui.texts.push_back({description.substr(split + 1),
                                anchor + glm::vec2(0, 12 * scale / viewport.y), 16 * textScale,
                                white, 0, cardTextWidth});
        }
        else
            ui.texts.push_back({description, anchor, 16 * textScale, white, 0, cardTextWidth});
        const auto label =
            maxed ? std::string("MAX RANK")
            : !unlocked
                ? std::format(
                      "NEEDS {} R1",
                      CPlayerProgression::skills()[static_cast<std::size_t>(skill.prerequisite)]
                          .name)
                : std::format("{} POINT{} / {}", m_progression.cost(skill.id),
                              m_progression.cost(skill.id) == 1 ? "" : "S",
                              available ? "UPGRADE" : "NEED POINTS");
        ui.texts.push_back({label, anchor + glm::vec2(0, 42 * scale / viewport.y), 15 * textScale,
                            accent, 0, cardTextWidth});
    }
    const bool retryHover = contains({.5f, .82f}, {340, 64}, cursor, viewport);
    ui.rectangles.push_back({{.5f, .82f}, {346, 70}, retryHover || m_selected == 6 ? white : cyan});
    ui.rectangles.push_back({{.5f, .82f}, {340, 64}, {.04f, .16f, .22f, 1}});
    ui.texts.push_back({"RETRY RUN", {.5f, .82f}, 27, white, 3, .8f});
    ui.texts.push_back({"1-4 BRANCH / CLICK OR ENTER BUY / TAB SELECT / R RETRY",
                        {.5f, .925f},
                        16,
                        muted,
                        0,
                        .94f});
    ui.texts.push_back(
        {"POINTS AND SKILLS LAST UNTIL YOU CLOSE THE GAME", {.5f, .965f}, 14, muted, 0, .94f});
    return ui;
}
