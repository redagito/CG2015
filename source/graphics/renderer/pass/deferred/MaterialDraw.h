#pragma once
#include <glm/glm.hpp>
class CMesh;
class CMaterial;
class IGraphicsResourceManager;
class CShaderProgram;
void bindMaterial(CMaterial *material, const IGraphicsResourceManager &manager,
                  CShaderProgram *shader);
void drawMaterialMesh(CMesh *mesh, const glm::mat4 &translation, const glm::mat4 &rotation,
                      const glm::mat4 &scale, CMaterial *material,
                      const IGraphicsResourceManager &manager, CShaderProgram *shader);
