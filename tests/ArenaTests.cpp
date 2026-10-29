#include "game/CGameWorld.h"
#include "game/arena/ArenaBoundary.h"
#include "game/arena/EnemySeparation.h"
#include "game/combat/CombatStatus.h"
#include "game/control/CArenaEnemyController.h"
#include "game/control/CFlightController.h"
#include "game/control/CFollowObjectController.h"
#include "game/control/CMothershipController.h"
#include "game/control/CSimpleWaypointController.h"
#include "game/encounter/EncounterProgress.h"
#include "game/flight/CChaseCamera.h"
#include "game/flight/FlightHud.h"
#include "resource/Resource.h"
#include "support/TestInput.h"
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main()
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        const auto arena = ArenaTuning::load(*resources);
        require(arena.mothership.approachSeconds == 60 && arena.mothership.waitSeconds == 5,
                "Bundled mothership deadline is not 60 + 5 seconds");
        require(arena.center.y - arena.halfExtents.y == 10 &&
                    arena.center.y + arena.halfExtents.y == 1010,
                "Expanded ceiling moved the floor or lost vertical flight room");
        require(ArenaBoundary::evaluate({0, 500, 0}, arena).strength == 0,
                "High-altitude flight still triggers the old ceiling");
        require(arena.templeScale == 24 && arena.mothership.scale == 400 &&
                    arena.mothership.destination.y == 750,
                "Landmark scale and approach altitude do not match the enlarged arena");
        // The lowest platform must be a closed, outward-facing box. Its underside
        // touches the ground; an inward-facing bottom competes with that ground.
        const auto templeMesh = resources->loadMesh("data/mesh/piramyde.obj");
        std::vector<float> vertices, normals, uv;
        std::vector<unsigned> indices;
        EPrimitiveType primitive;
        require(resources->getMesh(templeMesh, vertices, indices, normals, uv, primitive),
                "Temple geometry could not be loaded");
        const auto vertex = [&](unsigned index)
        {
            return glm::vec3(vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2]);
        };
        const auto normal = [&](unsigned index)
        { return glm::vec3(normals[index * 3], normals[index * 3 + 1], normals[index * 3 + 2]); };
        unsigned foundationFaces = 0;
        const glm::vec3 foundationCenter(0, (-.517224f - .346395f) * .5f, 0);
        const auto foundationVertex = [](glm::vec3 p)
        {
            return std::abs(std::abs(p.x) - 6.973486f) < .0001f &&
                   std::abs(std::abs(p.z) - 6.973486f) < .0001f && p.y >= -.5173f && p.y <= -.3463f;
        };
        for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            const auto a = vertex(indices[i]), b = vertex(indices[i + 1]),
                       c = vertex(indices[i + 2]);
            if (!foundationVertex(a) || !foundationVertex(b) || !foundationVertex(c))
                continue;
            const auto outward = glm::normalize(glm::cross(b - a, c - a));
            require(glm::dot(outward, (a + b + c) / 3.f - foundationCenter) > 0,
                    "Temple foundation has inward-facing triangles that fight the ground");
            for (unsigned j = 0; j < 3; ++j)
                require(glm::dot(outward, normal(indices[i + j])) > .99f,
                        "Temple foundation normals disagree with its outward faces");
            ++foundationFaces;
        }
        require(foundationFaces == 12, "Temple foundation regression did not cover all box faces");
        CGameObject mothership;
        mothership.setPosition({0, 68, 700});
        int deadlines = 0;
        CMothershipController approach({0, 68, 0}, arena.mothership, [&] { ++deadlines; });
        approach.attach(&mothership);
        approach.update(30);
        require(std::abs(mothership.getPosition().z - 350) < .001f &&
                    approach.approachRemaining() == 30 && approach.totalRemaining() == 35,
                "Mothership movement and countdown disagree at halfway");
        approach.update(31);
        require(glm::length(mothership.getPosition() - glm::vec3(0, 68, 0)) < .001f &&
                    glm::length(mothership.getVelocity()) == 0 &&
                    approach.approachRemaining() == 0 && approach.totalRemaining() == 4 &&
                    deadlines == 0,
                "Arrival did not stop movement or preserve the partial wait step");
        approach.update(3.5f);
        require(deadlines == 0 && approach.totalRemaining() == .5f,
                "Mothership triggered defeat early");
        approach.update(.5f);
        approach.update(1);
        require(deadlines == 1 && approach.totalRemaining() == 0,
                "Mothership deadline was missed or repeated");
        CMothershipController configurable({0, 68, 0}, {10, 0}, [&] { ++deadlines; });
        configurable.attach(&mothership);
        configurable.setActive(false);
        configurable.update(10);
        require(configurable.totalRemaining() == 10, "Inactive countdown kept running");
        configurable.setActive(true);
        configurable.update(10);
        require(deadlines == 2, "Configured approach with zero wait did not finish");
        auto invalid = arena;
        invalid.mothership.approachSeconds = 0;
        bool rejected = false;
        try
        {
            invalid.validate();
        }
        catch (const std::runtime_error &)
        {
            rejected = true;
        }
        require(rejected, "Zero mothership approach duration was accepted");
        require(ArenaBoundary::evaluate({0, 50, 0}, arena).strength == 0,
                "Normal low-altitude flight triggers an early warning");
        require(ArenaBoundary::evaluate({0, 20, 0}, arena).axis == 1 &&
                    ArenaBoundary::evaluate({0, 20, 0}, arena).strength > 0,
                "Approaching the floor does not trigger an altitude warning");
        require(ArenaBoundary::evaluate({770, 100, 0}, arena).axis == 0,
                "Horizontal edge warning was lost");
        require(ArenaBoundary::evaluate(arena.center, arena).strength == 0,
                "Arena center applies steering assistance");
        for (int axis = 0; axis < 3; ++axis)
            for (float sign : {-1.f, 1.f})
            {
                TestInput input;
                input.boost = true;
                input.throttleUp = true;
                CGameObject ship;
                auto start = arena.center;
                start[axis] += sign * (arena.halfExtents[axis] - arena.warningMargin * 1.1f);
                glm::vec3 outward(0);
                outward[axis] = sign;
                ship.setPosition(start);
                ship.setOrientation(glm::rotation(glm::vec3(0, 0, 1), outward));
                CFlightController flight(input, FlightTuning{}, &arena);
                flight.attach(&ship);
                bool warned = false, assisted = false;
                float outside = 0;
                for (int frame = 0; frame < 1200; ++frame)
                {
                    const auto before = ship.getPosition();
                    flight.update(1.f / 120);
                    require(glm::distance(before, ship.getPosition()) < 1,
                            "Boundary teleported or wrapped the player");
                    const auto excess =
                        glm::abs(ship.getPosition() - arena.center) - arena.halfExtents;
                    outside = std::max(outside, std::max(excess.x, std::max(excess.y, excess.z)));
                    warned |= flight.state().boundaryWarning > .05f;
                    assisted |= flight.state().boundaryAssist;
                }
                require(warned && assisted && outside < 15,
                        "Return assistance failed to keep boosted flight near the arena");
            }
        const std::vector<TargetSnapshot> approaching{{1, "A", {-20, 0, 0}, {20, 0, 0}},
                                                      {2, "B", {20, 0, 0}, {-20, 0, 0}}};
        const auto left = EnemySeparation::avoidance(
            1, approaching[0].position, approaching[0].velocity, approaching, arena.enemies);
        const auto right = EnemySeparation::avoidance(
            2, approaching[1].position, approaching[1].velocity, approaching, arena.enemies);
        require(glm::length(left) > .1f && glm::dot(left, right) < 0,
                "Crossing enemies did not choose opposing avoidance directions");
        CGameWorld flock;
        auto *flockPlayer = new CGameObject();
        flockPlayer->setPosition(arena.center + glm::vec3(0, 0, 140));
        const auto flockPlayerId = flock.addObject(flockPlayer);
        std::vector<GameObjectId> flockIds;
        for (int i = 0; i < 7; ++i)
        {
            auto *foe = new CGameObject();
            foe->setTargetName("FLOCK");
            foe->setPosition(arena.center);
            foe->addController(
                std::make_shared<CArenaEnemyController>(flock, flockPlayerId, arena, nullptr, 0));
            flockIds.push_back(flock.addObject(foe));
        }
        for (int frame = 0; frame < 360; ++frame)
            flock.update(1.f / 120);
        float separation = 0;
        unsigned pairs = 0;
        for (std::size_t a = 0; a < flockIds.size(); ++a)
            for (std::size_t b = a + 1; b < flockIds.size(); ++b)
            {
                const auto distance = glm::distance(flock.getObject(flockIds[a])->getPosition(),
                                                    flock.getObject(flockIds[b])->getPosition());
                require(std::isfinite(distance),
                        "Coincident enemies produced non-finite separation");
                separation += distance;
                ++pairs;
            }
        std::cout << "Seven-ship mean separation after three seconds: " << separation / pairs
                  << '\n';
        require(separation / pairs > 12, "Enemies remained bunched while pursuing the same target");
        CGameWorld world;
        auto *player = new CGameObject();
        player->setPosition(arena.center + glm::vec3(0, 0, 100));
        const auto playerId = world.addObject(player);
        CGameObject enemy;
        enemy.setPosition(arena.center);
        CArenaEnemyController ai(world, playerId, arena, nullptr);
        ai.attach(&enemy);
        bool fired = false;
        for (int frame = 0; frame < 120; ++frame)
        {
            ai.update(1.f / 60);
            fired |= ai.firing();
        }
        require(fired && enemy.getPosition().z > arena.center.z + 20,
                "Enemy failed to pursue and fire during an attack run");
        player->setPosition(arena.center + glm::vec3(70, 50, 50));
        player->setVelocity({-5, 0, 0});
        for (int frame = 0; frame < 120; ++frame)
            ai.update(1.f / 60);
        require(enemy.getPosition().y > arena.center.y + 5,
                "Enemy remained on a fixed altitude path");
        enemy.setPosition(arena.center);
        enemy.setOrientation(glm::quat(1, 0, 0, 0));
        ai.attach(&enemy);
        player->setPosition(arena.center + glm::vec3(0, 0, 10));
        ai.update(.016f);
        require(ai.mode() == EnemyMode::BreakAway && !ai.firing(),
                "Enemy did not break away before a close pass");
        for (int frame = 0; frame < 180; ++frame)
            ai.update(1.f / 60);
        require(glm::distance(enemy.getPosition(), player->getPosition()) > 25,
                "Enemy stayed glued to the player instead of extending its attack run");
        enemy.setPosition(arena.center + glm::vec3(arena.halfExtents.x - 1, 0, 0));
        ai.attach(&enemy);
        ai.update(.016f);
        require(ai.mode() == EnemyMode::Returning && !ai.firing(),
                "Enemy ignored the arena boundary");
        player->markDeleted();
        world.update(0);
        ai.update(.016f);
        require(!ai.firing(), "Enemy fired at a deleted player");
        auto *boss = new CGameObject();
        boss->setPosition({10, 20, 30});
        const auto bossId = world.addObject(boss);
        CGameObject ring;
        CFollowObjectController follow(world, bossId);
        follow.attach(&ring);
        follow.update(.1f);
        require(glm::distance(ring.getPosition(), boss->getPosition()) < .001f,
                "Boss decoration did not follow its owner");
        boss->markDeleted();
        world.update(0);
        follow.update(.1f);
        require(ring.isDeleteRequested(), "Boss decoration survived a deleted owner");
        CGameObject stopped;
        stopped.setPosition({1, 2, 3});
        CSimpleWaypointController waypoint({1, 2, 3}, 5, nullptr);
        waypoint.attach(&stopped);
        waypoint.update(.016f);
        require(glm::length(stopped.getVelocity()) == 0 && std::isfinite(stopped.getPosition().x),
                "Stationary waypoint produced a non-finite velocity");
        CChaseCamera camera(FlightTuning{});
        FlightState flightState;
        flightState.boundaryWarning = 1;
        flightState.boundaryAssist = true;
        camera.follow(arena.center, flightState, 0, 16.f / 9);
        CombatStatus status{200, 300, 2, 7};
        const auto hud = FlightHud::build(camera, arena.center, flightState, {1280, 720}, nullptr,
                                          nullptr, &arena, &status);
        bool warning = false, hull = false;
        for (const auto &text : hud.texts)
        {
            warning |= text.text.starts_with("RETURN ASSIST");
            hull |= text.text.starts_with("HULL 67%");
        }
        require(warning && hull && hud.rectangles.size() > 20,
                "Arena boundary outline, warning or hull display is missing");
        status.mothershipApproachRemaining = 30;
        status.mothershipTotalRemaining = 35;
        auto timerHud = FlightHud::build(camera, arena.center, flightState, {1280, 720}, nullptr,
                                         nullptr, &arena, &status);
        const auto hasTimer = [](const auto &overlay, const char *expected)
        {
            for (const auto &text : overlay.texts)
                if (text.text == expected)
                    return true;
            return false;
        };
        require(hasTimer(timerHud, "MOTHERSHIP IN 00:30 / STRIKE IN 00:35"),
                "Initial HUD countdown is missing");
        status.mothershipApproachRemaining = 0;
        status.mothershipTotalRemaining = 4.1f;
        timerHud = FlightHud::build(camera, arena.center, flightState, {1280, 720}, nullptr,
                                    nullptr, &arena, &status);
        require(hasTimer(timerHud, "MOTHERSHIP ARRIVED / STRIKE IN 00:05"),
                "HUD did not show the final wait or rounded down too early");
        status.finalGraceActive = true;
        timerHud = FlightHud::build(camera, arena.center, flightState, {1280, 720}, nullptr,
                                    nullptr, &arena, &status);
        require(hasTimer(timerHud, "FINAL CHANCE / CLEAR REMAINING ENEMIES"),
                "Grace-period HUD does not tell the player combat can still be won");

        EncounterProgress scoring{137, 0};
        for (int i = 0; i < 136; ++i)
            scoring.recordDefeat();
        require(!scoring.complete(), "Encounter won before its configured last enemy");
        scoring.recordDefeat();
        require(scoring.complete(), "Encounter total is still hardcoded");
        std::cout << "Arena: all six boundaries, boost return without teleportation, 3D "
                     "pursuit/attack/break-away, player lifetime, boss decoration and "
                     "seven-hostile victory passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Arena regression: " << e.what() << '\n';
        return 1;
    }
}
