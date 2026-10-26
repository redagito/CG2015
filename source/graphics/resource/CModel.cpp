#include "CModel.h"

CModel::CModel(const CMesh* mesh, const CMaterial* material, ResourceId meshId, ResourceId materialId)
	:
	m_mesh(mesh),
	m_material(material)
{
    m_meshId = meshId;
    m_materialId = materialId;
	return;
}

bool CModel::init(const CMesh* mesh, const CMaterial* material, ResourceId meshId, ResourceId materialId)
{
	if (mesh == nullptr || material == nullptr)
	{
		return false;
	}
	m_mesh = mesh;
	m_material = material;
    m_meshId = meshId;
    m_materialId = materialId;
	return true;
}

bool CModel::hasMesh() const
{
	return m_mesh != nullptr;
}

bool CModel::hasMaterial() const
{
	return m_material != nullptr;
}

const CMesh* CModel::getMesh() const
{
	return m_mesh;
}

const CMaterial* CModel::getMaterial() const
{
	return m_material;
}
