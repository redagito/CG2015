#include "audio/GameAudio.h"
#include "collision/CCollidable.h"
#include "collision/CCollisionSystem.h"
#include "game/CGameWorld.h"
#include "game/combat/CTargetingSystem.h"
#include "game/combat/CombatTuning.h"
#include "game/combat/FlightEngineSound.h"
#include "game/combat/ProjectileBallistics.h"
#include "game/control/CFlightAudioController.h"
#include "game/control/CFlightController.h"
#include "game/control/CProjectileController.h"
#include "game/control/CSimpleWaypointController.h"
#include "resource/Resource.h"
#include "support/RecordingAudioSystem.h"
#include "support/TestInput.h"
#include <glm/ext.hpp>
#include <iostream>
#include <stdexcept>
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
CAABBox box(glm::vec3 center, glm::vec3 half)
{
    CAABBox result;
    result.setMid(center);
    result.setHalfWidths(half);
    return result;
}
void checkIntercept(WeaponAim aim, glm::vec3 position, glm::vec3 velocity)
{
    const auto solution = ProjectileBallistics::intercept(aim, position, velocity);
    require(bool(solution), "Valid interception was rejected");
    const auto direction = glm::normalize(solution->aimPoint - aim.muzzle);
    require(
        glm::distance(aim.muzzle + (aim.inheritedVelocity + direction * aim.speed) * solution->time,
                      position + velocity * solution->time) < .001f,
        "Lead solution does not meet the actual inherited-velocity projectile");
}
} // namespace
int main()
{
    try
    {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        auto tuning = CombatTuning::load(*resources);
        auto invalid = tuning;
        invalid.weapon.muzzles.clear();
        bool rejected = false;
        try
        {
            invalid.validate();
        }
        catch (const std::runtime_error &)
        {
            rejected = true;
        }
        require(rejected, "Empty muzzle configuration accepted");
        CGameObject ship;
        ship.setPosition({10, 20, 30});
        ship.setScale({.5f, .5f, .5f});
        ship.setOrientation(glm::normalize(glm::quat(glm::vec3(.4f, .8f, 1.2f))));
        ship.setVelocity({5, 2, 20});
        const auto muzzle = ProjectileBallistics::muzzlePosition(ship, tuning.weapon.muzzles[0]);
        require(glm::distance(muzzle, ship.getPosition() + ship.getOrientation() *
                                                               (tuning.weapon.muzzles[0] * .5f)) <
                    .0001f,
                "Muzzle ignores roll, scale or ship position");
        WeaponAim aim{muzzle, ship.getForward(), ship.getVelocity(), tuning.weapon.projectileSpeed,
                      tuning.weapon.projectileLifetime};
        const auto launch = ProjectileBallistics::launch(aim);
        require(glm::distance(launch.velocity, ship.getVelocity() + ship.getForward() * aim.speed) <
                    .0001f,
                "Shot failed to inherit velocity");
        require(glm::distance(launch.orientation * glm::vec3(0, 1, 0),
                              glm::normalize(launch.velocity)) < .0001f,
                "Bullet long axis does not match flight path");
        aim = {{0, 0, 0}, {0, 0, 1}, {0, 0, 22}, 90, 4};
        checkIntercept(aim, {0, 0, 100}, {0, 0, 0});
        checkIntercept(aim, {0, 0, 100}, {20, 0, 0});
        checkIntercept(aim, {0, 0, 100}, {0, 0, 22});
        checkIntercept(aim, {0, 0, 100}, {0, 0, -68}); // linear-degenerate quadratic
        require(!ProjectileBallistics::intercept(aim, {0, 0, 100}, {0, 0, 200}),
                "Unreachable target has a lead solution");
        require(!ProjectileBallistics::intercept(aim, {0, 0, 1000}, {0, 0, 0}),
                "Lead ignores projectile lifetime");
        require(!ProjectileBallistics::intercept(aim, {0, 0, 0}, {0, 0, 0}),
                "Coincident target produced a singular lead");

        CGameWorld world;
        auto *enemy = new CGameObject();
        enemy->setTargetName("ONE");
        enemy->setPosition({0, 0, 50});
        const auto first = world.addObject(enemy);
        auto *second = new CGameObject();
        second->setTargetName("TWO");
        second->setPosition({20, 0, 80});
        const auto secondId = world.addObject(second);
        TestInput input;
        CTargetingSystem targeting(tuning.targeting);
        auto update = [&] { targeting.update(world.targets(), {0, 0, 0}, {0, 0, 1}, aim, input); };
        update();
        require(targeting.solution() && targeting.solution()->target.id == first,
                "Initial acquisition missed boresight target");
        input.targetNext = true;
        update();
        require(targeting.solution()->target.id == secondId, "Tab failed to cycle");
        update();
        require(targeting.solution()->target.id == secondId, "Held Tab cycled repeatedly");
        second->markDeleted();
        world.update(.016f);
        update();
        require(targeting.solution() && targeting.solution()->target.id == first,
                "Deleted target did not switch to the nearest survivor");
        input.targetNext = false;
        input.targetSelect = true;
        update();
        require(targeting.solution()->target.id == first, "Aim selection failed");
        input.targetSelect = false;
        input.targetClear = true;
        update();
        require(!targeting.solution(), "Clear failed");
        input.targetClear = false;
        update();
        require(!targeting.solution(), "Clear was immediately overridden by automatic acquisition");
        auto *third = new CGameObject();
        third->setTargetName("THREE");
        require(world.addObject(third) > secondId, "Object IDs were reused");

        {
            TestInput automaticInput;
            CTargetingSystem automatic(tuning.targeting);
            std::vector<TargetSnapshot> hostiles = {{10, "AHEAD", {0, 0, 100}, {}},
                                                    {20, "BEHIND", {0, 0, -5}, {}},
                                                    {30, "SIDE", {6, 0, 0}, {}}};
            const auto tick = [&]
            { automatic.update(hostiles, {0, 0, 0}, {0, 0, 1}, aim, automaticInput); };
            tick();
            require(automatic.solution()->target.id == 10, "Initial forward acquisition changed");
            hostiles.erase(hostiles.begin());
            tick();
            require(automatic.solution()->target.id == 20,
                    "Death retargeting ignored the closest enemy behind the player");
            hostiles.push_back({40, "CLOSER", {1, 0, 0}, {}});
            tick();
            require(automatic.solution()->target.id == 20,
                    "Living target was stolen by a nearer enemy");
            hostiles.erase(hostiles.begin());
            tick();
            require(automatic.solution()->target.id == 40,
                    "Death retargeting used spawn order instead of distance");
            hostiles.clear();
            tick();
            require(!automatic.solution(), "No survivors retained a stale target");
            hostiles = {{70, "NEXT WAVE BEHIND", {0, 0, -10}, {}},
                        {60, "NEXT WAVE AHEAD", {0, 0, 30}, {}}};
            tick();
            require(automatic.solution()->target.id == 70,
                    "Retargeting did not resume when the next wave spawned");
            hostiles = {{90, "TIE HIGH", {3, 0, 0}, {}}, {80, "TIE LOW", {-3, 0, 0}, {}}};
            tick();
            require(automatic.solution()->target.id == 80,
                    "Equal-distance retargeting was not stable by ID");
            automaticInput.targetClear = true;
            tick();
            automaticInput.targetClear = false;
            tick();
            require(!automatic.solution(), "Manual clear was overridden by death retargeting");
        }
        CCollisionSystem collisions;
        const auto friendly = collisions.getNewGroupId(), hostile = collisions.getNewGroupId();
        auto *near = collisions.add(box({0, 0, 5}, {.5f, .5f, .5f}), hostile);
        auto *far = collisions.add(box({0, 0, 10}, {.5f, .5f, .5f}), hostile);
        collisions.add(box({0, 0, 2}, {1, 1, 1}), friendly);
        const auto hit = collisions.trace({0, 0, 0}, {0, 0, 20}, {.1f, .1f, .1f}, friendly);
        require(hit.entity == near && hit.fraction < .25f,
                "Sweep missed the nearest target or hit the shooter group");
        CGameObject bullet;
        bullet.setCollidable(collisions.add(box({0, 0, 0}, {.1f, .1f, .1f}), friendly));
        bullet.getCollidable()->setDamage(50);
        bullet.getCollidable()->setCollidable(false);
        CProjectileController projectile({0, 0, 200}, 1, collisions, friendly);
        projectile.attach(&bullet);
        projectile.update(.1f);
        require(bullet.isDeleteRequested() && near->getDamageReceived() == 50 &&
                    far->getDamageReceived() == 0,
                "Fast shot tunneled or damaged multiple targets");
        CGameObject expiring;
        expiring.setCollidable(collisions.add(box({0, 0, 0}, {.1f, .1f, .1f}), friendly));
        expiring.getCollidable()->setCollidable(false);
        expiring.setPosition({100, 0, 0});
        CProjectileController expiry({0, 0, 10}, .1f, collisions, friendly);
        expiry.attach(&expiring);
        expiry.update(1);
        require(expiring.isDeleteRequested() && std::abs(expiring.getPosition().z - 1) < .001f,
                "Projectile outlived or overshot its lifetime");
        CCollidable transformed(0, box({2, 0, 0}, {2, 1, 3}));
        transformed.setTransform(
            {10, 0, 0}, glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)), {2, 1, .5f});
        require(glm::distance(transformed.getAABBox().getHalfWidths(), glm::vec3(1.5f, 1, 4)) <
                        .001f &&
                    glm::distance(transformed.getAABBox().getMid(), glm::vec3(10, 0, -4)) < .001f,
                "Collision bounds ignore model center, scale or rotation");

        CCollisionSystem movingCollisions;
        const auto sourceGroup = movingCollisions.getNewGroupId(),
                   targetGroup = movingCollisions.getNewGroupId();
        CGameWorld movingWorld;
        auto *sweptShot = new CGameObject();
        sweptShot->setProjectile(true);
        sweptShot->setCollidable(
            movingCollisions.add(box({0, 0, 0}, {.1f, .1f, .1f}), sourceGroup));
        sweptShot->getCollidable()->setCollidable(false);
        sweptShot->getCollidable()->setDamage(50);
        sweptShot->addController(std::make_shared<CProjectileController>(
            glm::vec3(0, 0, 200), 1, movingCollisions, sourceGroup));
        movingWorld.addObject(sweptShot);
        auto *crossingTarget = new CGameObject();
        crossingTarget->setPosition({-5, 0, 10});
        auto *movingCollider = movingCollisions.add(box({0, 0, 0}, {.5f, .5f, .5f}), targetGroup);
        crossingTarget->setCollidable(movingCollider);
        crossingTarget->addController(
            std::make_shared<CSimpleWaypointController>(glm::vec3(5, 0, 10), 100, nullptr));
        movingWorld.addObject(crossingTarget);
        movingCollisions.beginStep();
        movingWorld.update(.1f);
        require(!movingWorld.getObject(1) && movingCollider->getDamageReceived() == 50,
                "Projectile missed a crossing target updated later in spawn order");

        RecordingAudioSystem recorder;
        GameAudio audio(recorder);
        {
            TestInput controls;
            CGameObject player;
            auto flight = std::make_shared<CFlightController>(controls, FlightTuning{});
            player.addController(flight);
            player.addController(
                std::make_shared<CFlightAudioController>(&audio, flight, tuning.engine));
            controls.throttleDown = true;
            for (int i = 0; i < 240; ++i)
                player.update(1.f / 60);
            const float idleGain = recorder.gain, idlePitch = recorder.pitch;
            controls.throttleDown = false;
            controls.throttleUp = true;
            for (int i = 0; i < 240; ++i)
                player.update(1.f / 60);
            require(recorder.gain > idleGain * 2 && recorder.pitch > idlePitch + .2f,
                    "Engine ignores throttle");
            const float fullGain = recorder.gain, fullPitch = recorder.pitch;
            controls.boost = true;
            for (int i = 0; i < 180; ++i)
                player.update(1.f / 60);
            require(recorder.gain > fullGain * 1.2f && recorder.pitch > fullPitch + .2f,
                    "Engine ignores boost");
            controls.space = true;
            for (int i = 0; i < 180; ++i)
                player.update(1.f / 60);
            require(recorder.gain < fullGain * .5f && recorder.pitch < fullPitch,
                    "Braking did not spool down engine");
            player.setPosition({1000, 0, 0});
            player.update(.016f);
            require(recorder.gain < fullGain * .5f && recorder.starts == 1,
                    "Teleport made engine spike or restarted loop");
            player.setDead();
            player.update(.016f);
            require(!recorder.alive && recorder.stops == 1,
                    "Destroyed player kept its engine running during the explosion");
            player.update(.016f);
            require(recorder.starts == 1, "Destroyed player restarted its engine");
        }
        require(!recorder.alive && recorder.stops == 1,
                "Flight engine leaked after player destruction");
        std::cout << "Combat: muzzle transforms, inherited velocity, interception, target "
                     "lifetime/cycling, swept hits and throttle/boost audio passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Combat regression: " << e.what() << '\n';
        return 1;
    }
}
