#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
#include <map>
#include <tuple>
#include <limits>
#include "LoadMesh.h"
#include "util/StringUtil.h"
#include <spdlog/spdlog.h>

bool load(const std::string& file, SMesh& mesh)
{
    if (getFileExtension(file) == "obj") return loadMeshFromObj(file, mesh);
    SPDLOG_ERROR("Invalid or unknown mesh file format: {}", file.c_str());
    return false;
}

bool loadMeshFromObj(const std::string& file, SMesh& mesh)
{
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    if (!reader.ParseFromFile(file, config))
    {
        SPDLOG_ERROR("OBJ {}: {}", file.c_str(), reader.Error().c_str());
        return false;
    }
    if (!reader.Warning().empty()) SPDLOG_WARN("OBJ {}: {}", file.c_str(), reader.Warning().c_str());
    const auto& attrib = reader.GetAttrib();
    SMesh result;
    std::map<std::tuple<int, int, int>, unsigned int> vertices;
    std::vector<bool> missingNormals;
    // OBJ has independent position/normal/UV indices. Remap the complete tuple,
    // preserving UV seams and hard edges while combining all shapes.
    for (const auto& shape : reader.GetShapes())
    {
        for (const auto& index : shape.mesh.indices)
        {
            if (index.vertex_index < 0 || static_cast<size_t>(index.vertex_index) >= attrib.vertices.size() / 3 ||
                index.normal_index < -1 || index.texcoord_index < -1 ||
                (index.normal_index >= 0 && static_cast<size_t>(index.normal_index) >= attrib.normals.size() / 3) ||
                (index.texcoord_index >= 0 && static_cast<size_t>(index.texcoord_index) >= attrib.texcoords.size() / 2))
            {
                SPDLOG_ERROR("OBJ {} contains an out-of-range index.", file.c_str());
                return false;
            }
            auto key = std::make_tuple(index.vertex_index, index.normal_index, index.texcoord_index);
            auto [it, inserted] = vertices.try_emplace(key, static_cast<unsigned int>(vertices.size()));
            result.m_indices.push_back(it->second);
            if (!inserted) continue;
            for (int axis = 0; axis < 3; ++axis)
            {
                result.m_vertices.push_back(attrib.vertices[3 * static_cast<size_t>(index.vertex_index) + axis]);
                result.m_normals.push_back(index.normal_index < 0 ? 0.f : attrib.normals[3 * static_cast<size_t>(index.normal_index) + axis]);
            }
            for (int axis = 0; axis < 2; ++axis)
                result.m_uvs.push_back(index.texcoord_index < 0 ? 0.f : attrib.texcoords[2 * static_cast<size_t>(index.texcoord_index) + axis]);
            missingNormals.push_back(index.normal_index < 0);
        }
    }
    if (result.m_indices.empty() || result.m_indices.size() % 3 != 0)
    {
        SPDLOG_ERROR("OBJ {} contains no triangle mesh.", file.c_str());
        return false;
    }
    for (size_t i = 0; i < result.m_indices.size(); i += 3)
    {
        glm::vec3 p[3];
        for (int j = 0; j < 3; ++j)
        {
            const size_t offset = 3 * static_cast<size_t>(result.m_indices[i + j]);
            p[j] = glm::vec3(result.m_vertices[offset], result.m_vertices[offset + 1], result.m_vertices[offset + 2]);
        }
        const glm::vec3 normal = glm::cross(p[1] - p[0], p[2] - p[0]);
        for (int j = 0; j < 3; ++j)
        {
            const auto vertex = result.m_indices[i + j];
            if (missingNormals[vertex])
                for (int axis = 0; axis < 3; ++axis) result.m_normals[3 * static_cast<size_t>(vertex) + axis] += normal[axis];
        }
    }
    for (size_t i = 0; i < missingNormals.size(); ++i)
    {
        if (!missingNormals[i]) continue;
        glm::vec3 n(result.m_normals[3 * i], result.m_normals[3 * i + 1], result.m_normals[3 * i + 2]);
        n = glm::dot(n, n) > 0.f ? glm::normalize(n) : glm::vec3(0.f, 1.f, 0.f);
        for (int axis = 0; axis < 3; ++axis) result.m_normals[3 * i + axis] = n[axis];
    }
    result.m_type = EPrimitiveType::Triangle;
    mesh = std::move(result);
    return true;
}
