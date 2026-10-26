#include "game/CGameObject.h"
#include "game/control/CFlightController.h"
#include "game/flight/CChaseCamera.h"
#include "game/flight/FlightHud.h"
#include "resource/Resource.h"
#include "support/TestInput.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void simulate(CFlightController &flight, int frames, float step)
{
    for (int i = 0; i < frames; ++i)
        flight.update(step);
}
void finiteCamera(const ICamera &camera)
{
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            require(std::isfinite(camera.getView()[column][row]) &&
                        std::isfinite(camera.getProjection()[column][row]),
                    "Camera became singular");
}
} // namespace
int main()
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        auto tuning = FlightTuning::load(*resources);
        auto invalid = tuning;
        invalid.maxSpeed = invalid.minSpeed;
        bool rejected = false;
        try
        {
            invalid.validate();
        }
        catch (const std::runtime_error &)
        {
            rejected = true;
        }
        require(rejected, "Invalid flight tuning was accepted");
        invalid = tuning;
        invalid.pointerRadius = std::nanf("");
        rejected = false;
        try
        {
            invalid.validate();
        }
        catch (const std::runtime_error &)
        {
            rejected = true;
        }
        require(rejected, "Non-finite flight tuning was accepted");

        TestInput input;
        CGameObject ship;
        ship.setPosition({0, 80, 0});
        CFlightController flight(input, tuning);
        flight.attach(&ship);
        simulate(flight, 60, 1.f / 60);
        require(std::abs(ship.getPosition().y - 80) < .001f &&
                    std::abs(ship.getPosition().z - tuning.initialSpeed) < .01f,
                "Neutral flight drifted, changed speed or clamped altitude");
        input.pointer = {tuning.pointerRadius * .01f, 0};
        flight.update(.1f);
        require(glm::length(flight.state().steering) == 0,
                "Pointer dead zone did not suppress noise");
        input.pointer = {tuning.pointerRadius, 0};
        simulate(flight, 30, 1.f / 60);
        require(ship.getForward().x < -.4f, "Moving the mouse right did not steer screen-right");
        require(glm::dot(glm::normalize(flight.state().velocity), ship.getForward()) < .999f,
                "Assisted flight has no velocity lag");
        require(glm::distance(ship.getForward(), ship.getOrientation() * glm::vec3(0, 0, 1)) <
                    .0001f,
                "Ship rendering and flight orientation disagree");

        CGameObject climbing;
        CFlightController pitch(input, tuning);
        pitch.attach(&climbing);
        input.pointer = {0, -tuning.pointerRadius};
        simulate(pitch, 30, 1.f / 60);
        require(climbing.getForward().y > .3f, "Mouse-up did not pitch up");
        simulate(pitch, 900, 1.f / 120);
        require(std::abs(glm::length(pitch.state().orientation) - 1) < .0001f &&
                    std::isfinite(climbing.getPosition().y),
                "Full pitch loops broke quaternion flight");
        auto inverted = tuning;
        inverted.invertY = true;
        CGameObject invertedShip;
        CFlightController invertedFlight(input, inverted);
        invertedFlight.attach(&invertedShip);
        simulate(invertedFlight, 30, 1.f / 60);
        require(invertedShip.getForward().y < -.3f, "Inverted pitch option has no effect");

        input.pointer = {0, 0};
        input.rollRight = true;
        CGameObject rolling;
        CFlightController roll(input, tuning);
        roll.attach(&rolling);
        simulate(roll, 30, 1.f / 60);
        require((roll.state().orientation * glm::vec3(0, 1, 0)).x < -.4f,
                "Manual roll did not rotate the ship");
        input.rollRight = false;
        input.throttleUp = true;
        simulate(flight, 360, 1.f / 60);
        require(flight.state().throttle == 1 &&
                    std::abs(flight.state().speed - tuning.maxSpeed) < .001f,
                "Throttle failed to reach cruise maximum");
        input.boost = true;
        simulate(flight, 180, 1.f / 60);
        require(std::abs(flight.state().speed - tuning.boostSpeed) < .001f,
                "Boost failed to accelerate");
        input.space = true;
        simulate(flight, 180, 1.f / 60);
        require(flight.state().braking &&
                    std::abs(flight.state().speed - tuning.brakeSpeed) < .001f,
                "Brake did not override boost");
        input.boost = false;
        input.space = false;
        input.throttleUp = false;
        input.throttleDown = true;
        simulate(flight, 360, 1.f / 60);
        require(flight.state().throttle == 0 &&
                    std::abs(flight.state().speed - tuning.minSpeed) < .001f,
                "Throttle minimum failed");
        input.throttleDown = false;

        glm::vec3 referencePosition(0);
        glm::quat referenceOrientation(1, 0, 0, 0);
        for (int fps : {30, 60, 144})
        {
            TestInput controls;
            controls.pointer = {.12f, -.03f};
            controls.throttleUp = true;
            CGameObject object;
            CFlightController controller(controls, tuning);
            controller.attach(&object);
            simulate(controller, fps * 2, 1.f / fps);
            if (fps == 30)
            {
                referencePosition = object.getPosition();
                referenceOrientation = controller.state().orientation;
            }
            require(glm::distance(referencePosition, object.getPosition()) < .15f &&
                        std::abs(glm::dot(referenceOrientation, controller.state().orientation)) >
                            .9999f,
                    "Flight response depends strongly on frame rate");
        }
        glm::vec3 referenceEye(0);
        for (int fps : {30, 60, 144})
        {
            CChaseCamera chase(tuning);
            FlightState moving;
            chase.follow(glm::vec3(0), moving, 0, 16.f / 9);
            for (int frame = 1; frame <= fps * 2; ++frame)
                chase.follow({0, 0, 22.f * frame / fps}, moving, 1.f / fps, 16.f / 9);
            if (fps == 30)
                referenceEye = chase.getPosition();
            require(glm::distance(referenceEye, chase.getPosition()) < .4f,
                    "Camera lag depends strongly on frame rate");
        }
        CChaseCamera camera(tuning);
        FlightState state;
        camera.follow({0, 0, 0}, state, 0, 16.f / 9);
        finiteCamera(camera);
        const auto shipClip = camera.getProjection() * camera.getView() * glm::vec4(0, 0, 0, 1);
        require(shipClip.y / shipClip.w < 0, "Chase camera did not leave space above the ship");
        for (int i = 0; i < 240; ++i)
        {
            state.orientation = glm::angleAxis(float(i) * .04f, glm::normalize(glm::vec3(1, 0, 1)));
            camera.follow({0, 0, 0}, state, 1.f / 60, 16.f / 9);
            finiteCamera(camera);
        }
        camera.follow({200, 0, 0}, state, 1.f / 60, 16.f / 9);
        require(glm::distance(camera.getPosition(), glm::vec3(200, 0, 0)) < 15,
                "Camera swept across arena after wrapping");
        state = {};
        camera.reset();
        camera.follow({0, 0, 0}, state, 0, 16.f / 9);
        const float normalProjection = camera.getProjection()[1][1];
        state.boost = 1;
        for (int i = 0; i < 180; ++i)
            camera.follow({0, 0, 0}, state, 1.f / 60, 16.f / 9);
        require(camera.getProjection()[1][1] < normalProjection, "Boost FOV feedback is missing");
        for (auto viewport : {glm::vec2(1280, 720), glm::vec2(480, 900), glm::vec2(320, 180)})
        {
            camera.follow({0, 0, 0}, state, .016f, viewport.x / viewport.y);
            state.pointer = {tuning.pointerRadius, 0};
            const auto hud = FlightHud::build(camera, {0, 0, 0}, state, viewport);
            require(hud.rectangles.size() >= 11 && hud.texts.size() == 2,
                    "Flight reticles or controls are missing");
            const auto pointer = hud.rectangles.front().anchor;
            const float expected =
                .5f + tuning.pointerRadius * std::min(viewport.x, viewport.y) / viewport.x;
            require(std::abs(pointer.x - expected) < .0001f,
                    "Steering reticle changed physical size with aspect ratio");
            for (const auto &rect : hud.rectangles)
                require(std::isfinite(rect.anchor.x) && std::isfinite(rect.anchor.y),
                        "HUD contains non-finite positions");
        }
        flight.setActive(false);
        const auto stopped = ship.getPosition();
        flight.update(.1f);
        require(ship.getPosition() == stopped, "Disabled flight still updated");
        flight.detach();
        flight.update(.1f);
        std::cout << "Flight controls, frame-rate consistency, camera rolls/wraps, tuning and "
                     "reticles passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Flight regression: " << e.what() << '\n';
        return 1;
    }
}
