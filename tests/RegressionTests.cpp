#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "resource/core/LoadMesh.h"
#include "resource/core/SMesh.h"
#include "graphics/camera/CFreeCamera.h"
#include "util/Time.h"
#include "collision/CFrustum.h"
#include "graphics/camera/CFirstPersonCamera.h"
#include "graphics/scene/SSceneObject.h"
#include "math/CTransformer.h"
#include "graphics/renderer/core/ShadowMath.h"
#include "graphics/camera/CCamera.h"
#include "io/CShaderPreprocessor.h"
#include <filesystem>

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
int main()
{
    try
    {
        // Independent OBJ indices, multiple shapes, negative indices, triangulation,
        // missing normals/UVs, and transactional failure of an existing mesh.
        const char* path = "build/regression.obj";
        {
            std::ofstream file(path);
            file << "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
                    "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 1\n"
                    "o quad\nf 1/1/1 2/2/1 3/3/1 4/4/1\no triangle\nf -4 -3 -2\n";
        }
        SMesh mesh;
        require(loadMeshFromObj(path, mesh), "OBJ load failed");
        require(mesh.m_indices.size() == 9, "Shapes/triangulation lost faces");
        require(mesh.m_vertices.size() == mesh.m_normals.size(), "Normals not aligned");
        require(mesh.m_uvs.size() * 3 == mesh.m_vertices.size() * 2, "UVs not aligned");
        for (size_t i = 2; i < mesh.m_normals.size(); i += 3)
            require(std::abs(mesh.m_normals[i] - 1.f) < 0.0001f, "Incorrect generated normal");
        const auto original = mesh.m_vertices;
        { std::ofstream file(path); file << "v 0 0 0\nf 1 2 999999\n"; }
        require(!loadMeshFromObj(path, mesh), "Accepted invalid OBJ index");
        require(mesh.m_vertices == original, "Failed OBJ load corrupted existing mesh");
        CFreeCamera camera({0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 1.f, 0.f}, 90.f, 1.f, 0.1f, 100.f);
        require(std::abs(camera.getProjection()[1][1] - 1.f) < 0.0001f, "Degree FOV changed during GLM upgrade");
        const double before = getTime();
        require(getTime() >= before, "Clock moved backwards");
        CFrustum clip, corners;
        const auto view = glm::lookAt(glm::vec3(3.f, 2.f, 5.f), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
        const auto projection = glm::perspective(glm::radians(60.f), 1.5f, 0.1f, 20.f);
        clip.setFromViewProjectionClipSpaceApproach(view, projection);
        corners.setFromViewProjection(view, projection);
        for (int x = -8; x <= 8; ++x)
            for (int y = -8; y <= 8; ++y)
                for (int z = -8; z <= 8; ++z)
                {
                    glm::vec3 point(x + 0.123f, y + 0.234f, z + 0.345f);
                    const auto projected = projection * view * glm::vec4(point, 1.f);
                    const bool expected = projected.w > 0.f && glm::all(glm::lessThanEqual(glm::abs(glm::vec3(projected)), glm::vec3(projected.w)));
                    require(clip.isInside(point) == expected, "Clip-space frustum rejected a visible point");
                    require(corners.isInside(point) == expected, "Inverse-projection frustum used the wrong matrix convention");
                }
        clip.setFromViewProjectionClipSpaceApproach(glm::mat4(1.f), glm::ortho(-1.f, 1.f, -1.f, 1.f, 0.1f, 10.f));
        require(!clip.isInsideOrIntersects(CBoundingSphere({3.f, 1.2f, -2.f}, 0.5f)), "Frustum accepted a sphere outside another plane");
        SSceneObject mirrored(0, 0, {10.f, 0.f, 0.f}, glm::quat(1.f, 0.f, 0.f, 0.f), {-3.f, 1.f, 1.f}, true, CBoundingSphere({1.f, 0.f, 0.f}, 2.f));
        mirrored.updateBoundingSphere();
        require(std::abs(mirrored.boundingSphere.getRadius() - 6.f) < 0.0001f, "Mirrored bounds shrink or compound their scale");
        require(glm::distance(mirrored.boundingSphere.getPosition(), glm::vec3(7.f, 0.f, 0.f)) < 0.0001f, "Local bounding sphere center was not transformed");
        CFirstPersonCamera aiming({0.f, 0.f, 0.f}, {0.f, 1.f, -2.f}, {0.f, 1.f, 0.f}, 60.f, 1.f, 0.1f, 100.f);
        const auto aim = -glm::vec3(glm::inverse(aiming.getView())[2]);
        require(glm::distance(aim, glm::normalize(glm::vec3(0.f, 1.f, -2.f))) < 0.0001f, "First-person lookAt does not aim at its target");
        aiming.lookAt(glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
        require(std::isfinite(aiming.getView()[0][0]), "Vertical camera target made the view singular");
        CTransformer transform;
        transform.setRotation(glm::quat(0.f, 0.f, 0.f, 0.f));
        const auto inverse = transform.getInverseModelMatrix();
        require(std::isfinite(inverse[0][0]) && std::abs(inverse[0][0] - 1.f) < 0.0001f, "Zero/default quaternion broke inverse transforms");
        CCamera distant;
        const glm::vec3 eye(500.f,300.f,200.f),direction=glm::normalize(glm::vec3(-0.2f,-0.1f,-1.f));
        distant.setView(glm::lookAt(eye,eye+direction,glm::vec3(0,1,0)));
        distant.setProjection(glm::perspective(glm::radians(60.f),1.5f,0.1f,1000.f));
        distant.setPosition(eye);
        for (unsigned resolution : {64u,512u,4096u}) {
            const auto fit=fitDirectionalShadow(distant,glm::vec3(0,-1,0),100.f,resolution);
            const auto clip=fit.projection*fit.view*glm::vec4(eye+direction*20.f,1.f);
            require(glm::all(glm::lessThanEqual(glm::abs(glm::vec3(clip)/clip.w),glm::vec3(1.f))),"Fitted shadow map did not follow a distant camera");
            require(std::isfinite(fit.bias)&&fit.bias>0.f,"Fitted shadow bias is invalid");
        }
        std::filesystem::create_directories("build/include-tests");
        {std::ofstream f("build/include-tests/math.glsl");f<<"float example(){return 1.0;}\n";}
        CShaderPreprocessor preprocessor;preprocessor.setIncludePath("build/include-tests");
        std::string expanded;
        require(preprocessor.preprocess("#version 460 core\n#include \"math.glsl\"\nvoid main(){}",expanded)&&expanded.find("float example")!=std::string::npos&&expanded.find("#include")==std::string::npos,"Shader includes were not expanded into output");
        {std::ofstream f("build/include-tests/cycle.glsl");f<<"#include \"cycle.glsl\"\n";}
        require(!preprocessor.preprocess("#include \"cycle.glsl\"",expanded),"Cyclic includes were accepted");
        std::cout << "Regression checks passed.\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
