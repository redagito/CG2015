#pragma once

#include "CMesh.h"
#include "CMaterial.h"

/**
* \brief Model resource stores mesh and material.
*/
class CModel
{
public:
	CModel(const CMesh* mesh, const CMaterial* material, ResourceId meshId = invalidResource, ResourceId materialId = invalidResource);

	/**
	* \brief Initialize form mesh and material.
	*/
	bool init(const CMesh* mesh, const CMaterial* material, ResourceId meshId = invalidResource, ResourceId materialId = invalidResource);
    ResourceId getMeshId() const { return m_meshId; }
    ResourceId getMaterialId() const { return m_materialId; }

	bool hasMesh() const;
	bool hasMaterial() const;

	const CMesh* getMesh() const;
	const CMaterial* getMaterial() const;

private:
    ResourceId m_meshId = invalidResource, m_materialId = invalidResource;
	const CMesh* m_mesh = nullptr; /**< Mesh resource. */
	const CMaterial* m_material = nullptr; /**< Material resource. */
};
