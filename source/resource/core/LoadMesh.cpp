#include <tiny_obj_loader.h>

#include "LoadMesh.h"

#include "util/StringUtil.h"
#include "debug/Log.h"

bool load(const std::string& file, SMesh& mesh)
{
	std::string extension = getFileExtension(file);
	if (extension == "obj")
	{
		return loadMeshFromObj(file, mesh);
	}
	else
	{
		LOG_ERROR("Invalid or unknown mesh file format.");
		return false;
	}
}

bool loadMeshFromObj(const std::string& file, SMesh& mesh)
{
	// Wavefront OBJ file format loaded with tinyobj
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	// Load as obj
	std::string err = tinyobj::LoadObj(shapes, materials, file.c_str());

	if (!err.empty())
	{
		LOG_ERROR("%s.", err.c_str());
		return false;
	}
	if (shapes.size() > 1)
	{
		LOG_WARNING("Multple meshes in obj files not supported.");
		return false;
	}
	if (shapes.empty())
	{
		LOG_ERROR("No mesh data loaded from file %s.", file.c_str());
		return false;
	}
	mesh.m_vertices = shapes.at(0).mesh.positions;
	mesh.m_indices = shapes.at(0).mesh.indices;
	mesh.m_normals = shapes.at(0).mesh.normals;
	mesh.m_uvs = shapes.at(0).mesh.texcoords;
	mesh.m_type = EPrimitiveType::Triangle;
	return true;
}