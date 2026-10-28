#include "ParticlePass.h"
#include "graphics/ICamera.h"
#include "graphics/IGraphicsResourceManager.h"
#include "graphics/IScene.h"
#include "graphics/IWindow.h"
#include "graphics/renderer/core/CScopedRenderState.h"
#include "graphics/resource/CShaderProgram.h"
#include "resource/IResourceManager.h"
#include <algorithm>
#include <cmath>
#include <glm/ext.hpp>
ParticlePass::ParticlePass(ColorTarget &target) : m_target(target)
{
}
ParticlePass::~ParticlePass()
{
    if (m_buffer)
        glDeleteBuffers(1, &m_buffer);
    if (m_vao)
        glDeleteVertexArrays(1, &m_vao);
}
bool ParticlePass::init(IResourceManager &resources)
{
    m_shader = resources.loadShader("data/shader/deferred/particles.ini");
    if (m_shader == invalidResource)
        return false;
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_buffer);
    glVertexArrayVertexBuffer(m_vao, 0, m_buffer, 0, 10 * sizeof(float));
    const int counts[] = {3, 4, 2, 1}, offsets[] = {0, 3, 7, 9};
    for (unsigned location = 0; location < 4; ++location)
    {
        glEnableVertexArrayAttrib(m_vao, location);
        glVertexArrayAttribFormat(m_vao, location, counts[location], GL_FLOAT, GL_FALSE,
                                  offsets[location] * sizeof(float));
        glVertexArrayAttribBinding(m_vao, location, 0);
    }
    m_vertices.reserve(4096 * 60);
    return true;
}
void ParticlePass::render(const IScene &scene, const ICamera &camera, const IWindow &window,
                          const IGraphicsResourceManager &resources)
{
    if (scene.particles().empty())
        return;
    CScopedRenderState state;
    auto *shader = resources.getShaderProgram(m_shader);
    if (!shader)
        return;
    m_target.framebuffer.setActive(GL_FRAMEBUFFER);
    glViewport(0, 0, window.getWidth(), window.getHeight());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    shader->setActive();
    shader->setUniform("view_projection", camera.getProjection() * camera.getView());
    const auto inverse = glm::inverse(camera.getView());
    const glm::vec3 right(inverse[0]), up(inverse[1]);
    const glm::vec2 corners[] = {{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}};
    // Smoke needs back-to-front blending; luminous particles can accumulate freely.
    for (bool smoke : {true, false})
    {
        std::vector<const RenderParticle *> particles;
        for (const auto &particle : scene.particles())
            if ((particle.kind == ParticleKind::Smoke) == smoke && particles.size() < 4096)
                particles.push_back(&particle);
        if (smoke)
            std::stable_sort(particles.begin(), particles.end(),
                             [&](const auto *a, const auto *b)
                             {
                                 return glm::distance(a->position, camera.getPosition()) >
                                        glm::distance(b->position, camera.getPosition());
                             });
        m_vertices.clear();
        for (const auto *particle : particles)
        {
            float angle = particle->rotation;
            if (particle->kind == ParticleKind::Streak)
                angle = std::atan2(glm::dot(particle->velocity, up),
                                   glm::dot(particle->velocity, right));
            auto axisX = right * std::cos(angle) + up * std::sin(angle),
                 axisY = -right * std::sin(angle) + up * std::cos(angle);
            auto center = particle->position;
            auto size = particle->halfSize;
            if (particle->kind == ParticleKind::Beam)
            {
                // A world-space ribbon joins the exact endpoints, independently of view angle.
                const auto delta = particle->velocity;
                if (glm::dot(delta, delta) < 1e-8f)
                    continue;
                center += delta * .5f;
                auto side = glm::cross(delta, camera.getPosition() - center);
                if (glm::dot(side, side) < 1e-8f)
                    side = glm::cross(delta, up);
                if (glm::dot(side, side) < 1e-8f)
                    side = glm::cross(delta, right);
                axisX = delta * .5f;
                axisY = glm::normalize(side);
                size = {1, particle->halfSize.x};
            }
            for (const auto &corner : corners)
            {
                const auto position =
                    center + axisX * corner.x * size.x + axisY * corner.y * size.y;
                m_vertices.insert(m_vertices.end(),
                                  {position.x, position.y, position.z, particle->color.r,
                                   particle->color.g, particle->color.b, particle->color.a,
                                   corner.x, corner.y, float(particle->kind)});
            }
        }
        if (m_vertices.empty())
            continue;
        glBlendFunc(GL_SRC_ALPHA, smoke ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE);
        glNamedBufferData(m_buffer, m_vertices.size() * sizeof(float), m_vertices.data(),
                          GL_STREAM_DRAW);
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(m_vertices.size() / 10));
    }
}
