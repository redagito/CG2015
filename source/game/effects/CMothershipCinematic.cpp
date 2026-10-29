#include "CMothershipCinematic.h"
#include "audio/GameAudio.h"
#include "game/CGameObject.h"
#include "graphics/IScene.h"
#include "graphics/effects/RenderParticle.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
#include <vector>
namespace
{
constexpr float chargeSeconds = 1.2f;
constexpr float beamSeconds = 2.3f;
constexpr float destructionTime = chargeSeconds + beamSeconds;
constexpr float duration = destructionTime + 2.5f;
} // namespace
CMothershipCinematic::CMothershipCinematic(IScene &scene, CGameObject &temple, glm::vec3 emitter,
                                           glm::vec3 impact, GameAudio *audio)
    : m_scene(scene), m_temple(temple), m_emitter(emitter), m_impact(impact), m_audio(audio)
{
    // Scale the destruction presentation with the temple, not with the fighter models.
    m_effectScale = std::max(1.f, temple.getScale().y / 3.f);
    auto &features = m_camera.getFeatureInfoForWrite();
    features.lenseFlareActive = features.godRayActive = features.dofActive = false;
    m_sourceLight = scene.createPointLight(emitter, 80 * m_effectScale, {.15f, 1, .55f}, 0, false);
    m_impactLight = scene.createPointLight(impact + glm::vec3(0, 4 * m_effectScale, 0),
                                           100 * m_effectScale, {1, .4f, .1f}, 0, false);
    if (m_audio)
        m_audio->emit({GameAudioEventType::MothershipCharge, emitter});
}
CMothershipCinematic::~CMothershipCinematic()
{
    // Light slots belong to this scene; disable them before the next state takes over.
    m_scene.setPointLight(m_sourceLight, m_emitter, 80 * m_effectScale, {.15f, 1, .55f}, 0, false);
    m_scene.setPointLight(m_impactLight, m_impact, 100 * m_effectScale, {1, .4f, .1f}, 0, false);
    m_scene.setParticles({});
}
void CMothershipCinematic::update(float seconds, glm::vec2 viewport)
{
    if (!std::isfinite(seconds) || seconds < 0)
        return;
    m_time = std::min(duration, m_time + seconds);
    if (m_time >= chargeSeconds && !m_fired)
    {
        m_fired = true;
        if (m_audio)
            m_audio->emit({GameAudioEventType::MothershipBeam, m_emitter});
    }
    if (m_time >= destructionTime && !m_destroyed)
    {
        m_destroyed = true;
        m_temple.setVisible(false);
        if (m_audio)
            m_audio->emit({GameAudioEventType::Destroyed, m_impact});
    }
    updateCamera(viewport);
    publishEffects();
}
void CMothershipCinematic::updateCamera(glm::vec2 viewport)
{
    const float aspect = std::max(1.f, viewport.x) / std::max(1.f, viewport.y);
    const auto midpoint = (m_emitter + m_impact) * .5f;
    const float orbit = .55f + .16f * (m_time / duration);
    // Pull back in portrait windows to keep the UFO's broad disc and temple in frame.
    const float distance =
        std::max(240 * m_effectScale, glm::distance(m_emitter, m_impact) * 1.2f) *
        std::max(1.f, 1.25f / aspect);
    auto eye = midpoint + glm::vec3(std::sin(orbit) * distance, 30 * m_effectScale,
                                    std::cos(orbit) * distance);
    const float shake = beamActive()  ? .10f
                        : m_destroyed ? .45f * std::exp(-(m_time - destructionTime) * 3)
                                      : 0;
    eye += glm::vec3(std::sin(m_time * 67), std::cos(m_time * 53), 0) * shake;
    m_camera.setPosition(eye);
    m_camera.setView(glm::lookAt(eye, midpoint, glm::vec3(0, 1, 0)));
    m_camera.setProjection(glm::perspective(glm::radians(50.f), aspect, .5f, 12000.f));
    if (m_audio)
    {
        const auto inverse = glm::inverse(m_camera.getView());
        m_audio->setListener(eye, -glm::vec3(inverse[2]), glm::vec3(inverse[1]));
    }
}
void CMothershipCinematic::publishEffects()
{
    std::vector<RenderParticle> particles;
    const auto glow =
        [&](glm::vec3 position, float size, glm::vec4 color, ParticleKind kind = ParticleKind::Glow)
    { particles.push_back({position, {}, color, {size, size}, 0, kind}); };
    const float charge = glm::clamp(m_time / chargeSeconds, 0.f, 1.f);
    const float pulse = .9f + .1f * std::sin(m_time * 57);
    if (!m_destroyed)
    {
        glow(m_emitter, 2 + charge * 5, {.4f, 4, 1.8f, charge * .8f});
        glow(m_emitter, 1 + charge * 2, {5, 8, 6, charge});
        // Converging energy motes make the charge-up readable before the beam appears.
        for (int i = 0; i < 24; ++i)
        {
            const float angle = i * 2.399963f + m_time * 2;
            const float radius = 3 + (1 - charge) * 9;
            glow(m_emitter + glm::vec3(std::cos(angle) * radius, std::sin(angle * 1.7f) * 2,
                                       std::sin(angle) * radius),
                 .3f, {.2f, 3, 1, charge});
        }
    }
    if (beamActive())
    {
        const auto delta = m_impact - m_emitter;
        particles.push_back(
            {m_emitter, delta, {.1f, 4, 1.1f, .65f}, {3.5f * pulse, 1}, 0, ParticleKind::Beam});
        particles.push_back(
            {m_emitter, delta, {7, 10, 8, 1}, {.65f * pulse, 1}, 0, ParticleKind::Beam});
        glow(m_impact, 7, {5, 6, 2, 1});
        glow(m_impact, 12, {.4f, 3, 1, .7f}, ParticleKind::Shockwave);
    }
    // Sparks climb from the strike point, followed by a larger destruction burst and ash.
    const float strikeAge = m_time - chargeSeconds;
    if (strikeAge >= 0)
    {
        for (int i = 0; i < 100; ++i)
        {
            const float angle = i * 2.399963f;
            const float age =
                m_destroyed ? m_time - destructionTime : std::fmod(strikeAge + i * .021f, .7f);
            const glm::vec3 velocity(std::cos(angle) * (8 + i % 7), 8 + i % 13,
                                     std::sin(angle) * (8 + i % 7));
            const auto position = m_impact + velocity * age + glm::vec3(0, -4 * age * age, 0);
            const float fade = std::max(0.f, 1 - age / 2.5f);
            particles.push_back({position,
                                 velocity,
                                 {3, .8f, .12f, fade},
                                 {.4f, .2f},
                                 angle,
                                 ParticleKind::Debris});
        }
    }
    if (m_destroyed)
    {
        const float age = m_time - destructionTime;
        glow(m_impact, 8 + age * 22, {8, 4, 1, std::max(0.f, 1 - age / .55f)});
        glow(m_impact, 8 + age * 25, {4, 1, .15f, std::max(0.f, 1 - age / 1.5f)},
             ParticleKind::Shockwave);
        for (int i = 0; i < 18; ++i)
        {
            const float angle = i * 2.399963f;
            glow(m_impact + glm::vec3(std::cos(angle) * age * 6, age * 8 + i * .3f,
                                      std::sin(angle) * age * 6),
                 3 + age * 4, {.12f, .13f, .14f, .4f}, ParticleKind::Smoke);
        }
    }
    m_scene.setPointLight(m_sourceLight, m_emitter, 80 * m_effectScale, {.15f, 1, .55f},
                          m_destroyed ? 0 : charge * 5, false);
    const float impactIntensity = beamActive()  ? 10
                                  : m_destroyed ? 16 * std::exp(-(m_time - destructionTime) * 3)
                                                : 0;
    m_scene.setPointLight(m_impactLight, m_impact + glm::vec3(0, 4 * m_effectScale, 0),
                          100 * m_effectScale, {1, .5f, .15f}, impactIntensity, false);
    // Expand endpoint-local effects while keeping each beam aimed at the real impact point.
    for (auto &particle : particles)
    {
        if (particle.kind != ParticleKind::Beam)
        {
            const auto origin = glm::distance(particle.position, m_emitter) <
                                        glm::distance(particle.position, m_impact)
                                    ? m_emitter
                                    : m_impact;
            particle.position = origin + (particle.position - origin) * m_effectScale;
            particle.velocity *= m_effectScale;
        }
        particle.halfSize *= m_effectScale;
    }
    m_scene.setParticles(particles);
}
UiDocument CMothershipCinematic::overlay(glm::vec2 viewport) const
{
    UiDocument ui;
    const float scale = std::max(.01f, std::min(viewport.x / 1280.f, viewport.y / 720.f));
    const glm::vec2 bar(viewport.x / scale, viewport.y / scale * .08f);
    ui.rectangles.push_back({{.5f, .04f}, bar, {0, 0, 0, 1}});
    ui.rectangles.push_back({{.5f, .96f}, bar, {0, 0, 0, 1}});
    const char *caption = m_destroyed    ? "THE TEMPLE HAS FALLEN"
                          : beamActive() ? "TEMPLE UNDER ATTACK"
                                         : "MOTHERSHIP WEAPON CHARGING";
    const float fade = finished() ? 1.f : glm::clamp((m_time - (duration - .6f)) / .6f, 0.f, 1.f);
    ui.texts.push_back({caption, {.5f, .88f}, 24, {.8f, 1, .85f, 1 - fade}, 2, .9f});
    if (fade > 0)
        ui.rectangles.push_back({{.5f, .5f}, viewport / scale, {0, 0, 0, fade}});
    return ui;
}
CCamera &CMothershipCinematic::camera()
{
    return m_camera;
}
bool CMothershipCinematic::finished() const
{
    return m_time >= duration;
}
bool CMothershipCinematic::beamActive() const
{
    return m_time >= chargeSeconds && m_time < destructionTime;
}
