#include "FlightHud.h"
#include "FlightState.h"
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
                            glm::vec2 viewport)
{
    UiDocument ui;
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
    if (project(camera, position + flight.orientation * glm::vec3(0, 0, 100), heading))
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
    return ui;
}
