#include "FlightHud.h"
#include "FlightState.h"
#include "game/arena/ArenaTuning.h"
#include "game/combat/CombatStatus.h"
#include "game/combat/TargetingSolution.h"
#include "game/combat/WeaponAim.h"
#include "game/effects/PlayerFeedback.h"
#include "graphics/ICamera.h"
#include <algorithm>
#include <cmath>
#include <format>

bool FlightHud::project(const ICamera &camera, glm::vec3 point, glm::vec2 &anchor)
{
    const auto clip = camera.getProjection() * camera.getView() * glm::vec4(point, 1);
    if (clip.w <= .001f || !std::isfinite(clip.w))
        return false;
    const auto ndc = glm::vec2(clip) / clip.w;
    anchor = glm::vec2(.5f + ndc.x * .5f, .5f - ndc.y * .5f);
    return std::isfinite(anchor.x) && std::isfinite(anchor.y) && anchor.x > .02f &&
           anchor.x < .98f && anchor.y > .02f && anchor.y < .98f;
}
UiDocument FlightHud::build(const ICamera &camera, glm::vec3 position, const FlightState &flight,
                            glm::vec2 viewport, const WeaponAim *weapon,
                            const TargetingSolution *target, const ArenaTuning *arena,
                            const CombatStatus *status, const PlayerFeedback *feedback)
{
    UiDocument ui;
    if (arena)
    {
        // Projected holographic edges make the full flight volume visible above the ground art.
        const glm::vec4 edge = flight.boundaryWarning > .05f ? glm::vec4(1, .55f, .15f, .6f)
                                                             : glm::vec4(.25f, .65f, .9f, .3f);
        for (int axis = 0; axis < 3; ++axis)
            for (int a : {-1, 1})
                for (int b : {-1, 1})
                    for (int sample = 0; sample <= 24; ++sample)
                    {
                        glm::vec3 local(0);
                        local[axis] = glm::mix(-arena->halfExtents[axis], arena->halfExtents[axis],
                                               sample / 24.f);
                        local[(axis + 1) % 3] = a * arena->halfExtents[(axis + 1) % 3];
                        local[(axis + 2) % 3] = b * arena->halfExtents[(axis + 2) % 3];
                        glm::vec2 anchor;
                        if (project(camera, arena->center + local, anchor))
                            ui.rectangles.push_back({anchor, {2, 2}, edge});
                    }
    }
    const glm::vec4 white(.95f, .98f, 1, 1), cyan(.25f, .85f, .95f, .9f), amber(1, .72f, .2f, .9f);
    const float shortest = std::max(1.f, std::min(viewport.x, viewport.y));
    const auto steer =
        glm::vec2(.5f) + flight.pointer * shortest / glm::max(viewport, glm::vec2(1));
    // Hollow steering marker leaves targets visible; the center tick is neutral input.
    for (float side : {-1.f, 1.f})
    {
        ui.rectangles.push_back({steer, {10, 2}, cyan, {side * 9, -10}});
        ui.rectangles.push_back({steer, {10, 2}, cyan, {side * 9, 10}});
        ui.rectangles.push_back({steer, {2, 20}, cyan, {side * 14, 0}});
    }
    ui.rectangles.push_back({{.5f, .5f}, {3, 3}, {.3f, .65f, .7f, .7f}});
    glm::vec2 heading;
    if (project(camera,
                weapon ? weapon->muzzle + weapon->forward * 100.f
                       : position + flight.orientation * glm::vec3(0, 0, 100),
                heading))
    {
        ui.rectangles.push_back({heading, {7, 2}, white, {-7, 0}});
        ui.rectangles.push_back({heading, {7, 2}, white, {7, 0}});
        ui.rectangles.push_back({heading, {2, 7}, white, {0, -7}});
        ui.rectangles.push_back({heading, {2, 7}, white, {0, 7}});
    }
    glm::vec2 velocity;
    if (glm::dot(flight.velocity, flight.velocity) > .01f &&
        project(camera, position + glm::normalize(flight.velocity) * 100.f, velocity))
    {
        ui.rectangles.push_back({velocity, {3, 3}, amber});
    }
    // Target boxes are distinct from the steering marker and the green lead pipper.
    const glm::vec4 targetColor(1, .35f, .25f, 1), leadColor(.4f, 1, .35f, 1);
    std::string targetStatus = "NO TARGET   TAB / NEXT   T / AIM SELECT   X / CLEAR";
    if (target)
    {
        glm::vec2 anchor;
        const bool onScreen = project(camera, target->target.position, anchor);
        if (!onScreen)
        {
            const auto clip =
                camera.getProjection() * camera.getView() * glm::vec4(target->target.position, 1);
            glm::vec2 direction(clip.x, -clip.y);
            if (glm::dot(direction, direction) < 1e-6f)
                direction = glm::vec2(0, 1);
            direction = glm::normalize(direction);
            const float horizontal =
                std::abs(direction.x) > .001f ? .44f / std::abs(direction.x) : 1000;
            const float vertical =
                std::abs(direction.y) > .001f ? .34f / std::abs(direction.y) : 1000;
            anchor = glm::vec2(.5f) + direction * std::min(horizontal, vertical);
        }
        for (float x : {-1.f, 1.f})
            for (float y : {-1.f, 1.f})
            {
                ui.rectangles.push_back({anchor, {10, 2}, targetColor, {x * 17, y * 22}});
                ui.rectangles.push_back({anchor, {2, 10}, targetColor, {x * 22, y * 17}});
            }
        targetStatus = std::format("{}   RANGE {:.0f}{}", target->target.name, target->range,
                                   onScreen ? "" : "   OFF SCREEN");
        if (target->intercept)
        {
            glm::vec2 lead;
            if (project(camera, target->intercept->aimPoint, lead))
            {
                ui.rectangles.push_back({lead, {12, 2}, leadColor, {0, -6}});
                ui.rectangles.push_back({lead, {12, 2}, leadColor, {0, 6}});
                ui.rectangles.push_back({lead, {2, 12}, leadColor, {-6, 0}});
                ui.rectangles.push_back({lead, {2, 12}, leadColor, {6, 0}});
                targetStatus += std::format("   LEAD {:.1f}s", target->intercept->time);
            }
            else
                targetStatus += "   LEAD OFF SCREEN";
        }
        else
            targetStatus += "   NO FIRING SOLUTION";
    }
    // Keep thin reticles visible in small/portrait windows despite UI design scaling.
    const float uiScale = std::max(.01f, std::min(viewport.x / 1280.f, viewport.y / 720.f));
    const float markerScale = std::max(1.f, .75f / uiScale);
    for (auto &rectangle : ui.rectangles)
    {
        rectangle.size *= markerScale;
        rectangle.offset *= markerScale;
    }
    ui.texts = {{std::format("SPEED {:02.0f}    THROTTLE {:.0f}%{}", glm::length(flight.velocity),
                             flight.throttle * 100,
                             flight.braking       ? "    BRAKE"
                             : flight.boost > .5f ? "    BOOST"
                                                  : ""),
                 {.5f, .90f},
                 20,
                 white,
                 1,
                 .9f},
                {"MOUSE / STEER   W S / THROTTLE   A D / ROLL   SHIFT / BOOST   SPACE / BRAKE   "
                 "LMB / FIRE   R / CENTER",
                 {.5f, .96f},
                 15,
                 cyan,
                 0,
                 .94f}};
    ui.texts.push_back({targetStatus, {.5f, .065f}, 18, target ? targetColor : cyan, 1, .92f});
    if (flight.boundaryWarning > .05f)
        ui.texts.push_back(
            {flight.boundaryAssist ? "RETURN ASSIST / TURNING INTO ARENA" : "BOUNDARY / TURN BACK",
             {.5f, .15f},
             20,
             {1, .6f, .2f, 1},
             1,
             .9f});
    if (status)
        ui.texts.push_back({std::format("HULL {:.0f}%    HOSTILES DEFEATED {} / {}",
                                        100.f * status->health / std::max(1.f, status->maxHealth),
                                        status->defeated, status->total),
                            {.5f, .84f},
                            18,
                            {.85f, .95f, 1, 1},
                            1,
                            .9f});
    if (status && status->waveCount > 0)
        ui.texts.push_back(
            {std::format("WAVE {} / {}    ENEMIES LEFT {}    SKILL POINTS {}", status->wave,
                         status->waveCount, status->waveRemaining, status->points),
             {.5f, .19f},
             18,
             cyan,
             1,
             .94f});
    if (status && status->mothershipTotalRemaining >= 0)
    {
        // Round up: a positive fraction of a second must never read as zero.
        const auto clock = [](float seconds)
        {
            const int ticks = int(std::ceil(std::max(0.f, seconds)));
            return std::format("{:02}:{:02}", ticks / 60, ticks % 60);
        };
        const auto phase =
            status->mothershipApproachRemaining > 0
                ? std::format("MOTHERSHIP IN {}", clock(status->mothershipApproachRemaining))
                : std::string("MOTHERSHIP ARRIVED");
        ui.texts.push_back(
            {std::format("{} / STRIKE IN {}", phase, clock(status->mothershipTotalRemaining)),
             {.5f, .105f},
             18,
             status->mothershipTotalRemaining <= 10 ? glm::vec4(1, .3f, .15f, 1) : cyan,
             1,
             .94f});
    }
    if (feedback)
    {
        if (feedback->damage > .01f)
        {
            ui.rectangles.push_back(
                {{.5f, .5f}, viewport / uiScale, {.65f, .015f, .01f, .12f * feedback->damage}});
            const auto local =
                glm::vec3(camera.getView() * glm::vec4(feedback->incomingDirection, 0));
            glm::vec2 side;
            glm::vec2 size;
            if (std::abs(local.x) > .25f && std::abs(local.x) > std::abs(local.y))
            {
                side = {local.x > 0 ? .98f : .02f, .5f};
                size = {8, 120};
            }
            else
            {
                side = {.5f, local.z > 0 || local.y < 0 ? .98f : .02f};
                size = {120, 8};
            }
            ui.rectangles.push_back({side, size, {1, .2f, .08f, feedback->damage * .85f}});
        }
        if (feedback->hitConfirm > 0)
        {
            const auto anchor = glm::clamp(heading, glm::vec2(.06f), glm::vec2(.94f));
            for (float x : {-1.f, 1.f})
                for (float y : {-1.f, 1.f})
                    ui.rectangles.push_back({anchor,
                                             {4, 4},
                                             {1, 1, 1, std::min(1.f, feedback->hitConfirm / .08f)},
                                             {x * 7, y * 7}});
        }
    }
    if (status && status->finalDeadlineExpired)
        ui.texts.push_back(
            {"TIME EXPIRED / TEMPLE STRIKE IMMINENT", {.5f, .26f}, 24, {1, .3f, .15f, 1}, 1, .94f});
    if (status && status->health <= 0)
        ui.texts.push_back({"SHIP DESTROYED", {.5f, .22f}, 30, {1, .35f, .2f, 1}, 3, .9f});
    else if (status && status->total > 0 && status->defeated >= status->total)
        ui.texts.push_back({"AREA SECURED", {.5f, .22f}, 30, {.5f, 1, .5f, 1}, 3, .9f});
    return ui;
}
