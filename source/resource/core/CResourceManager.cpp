#include "CResourceManager.h"
#include "AsyncResourceState.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "lodepng.h"
#include "tiny_obj_loader.h"

#include "resource/IResourceListener.h"

#include "io/JsonUtil.h"
#include "io/JsonDeserialize.h"
#include "io/CIniFile.h"
#include "io/CShaderPreprocessor.h"

#include "LoadMaterial.h"
#include "LoadShader.h"
#include "LoadMesh.h"
#include "LoadImage.h"
#include "LoadModel.h"

#include <spdlog/spdlog.h>

CResourceManager::CResourceManager()
{
    return;
}

CResourceManager::~CResourceManager()
{
    stopAsync();
	for (auto listener : m_resourceListeners)
	{
		listener->onDetach(this);
	}
}

ResourceId CResourceManager::createMesh(const std::vector<float>& vertices,
                                        const std::vector<unsigned int>& indices,
                                        const std::vector<float>& normals,
                                        const std::vector<float>& uvs, EPrimitiveType type)
{
    ensureOwner();
    // Create mesh id
    ResourceId id = m_nextMeshId;
    ++m_nextMeshId;

    // Add mesh
    m_meshes[id] = SMesh(vertices, indices, normals, uvs, type);

    // Notify listener with create event
    notifyResourceListeners(EResourceType::Mesh, id, EListenerEvent::Create);
    return id;
}

ResourceId CResourceManager::loadMesh(const std::string& file)
{
    ensureOwner();
    auto entry = m_meshFiles.find(file);
    if (entry != m_meshFiles.end())
    {
        return entry->second;
    }
    requireCacheHit(file);

    // Load mesh
	SMesh mesh;
	if (!load(file, mesh))
	{
		SPDLOG_ERROR("Failed to load mesh from file {}.", file.c_str());
		return invalidResource;
	}

    // Create mesh resource
    ResourceId meshId = createMesh(mesh.m_vertices, mesh.m_indices, mesh.m_normals, mesh.m_uvs, mesh.m_type);
    if (meshId == invalidResource)
    {
        SPDLOG_ERROR("Failed to create mesh resource id from file {}.", file.c_str());
        return invalidResource;
    }
    m_meshFiles[file] = meshId;
    return meshId;
}

bool CResourceManager::getMesh(ResourceId id, std::vector<float>& vertices,
                               std::vector<unsigned int>& indices, std::vector<float>& normals,
                               std::vector<float>& uvs, EPrimitiveType& type) const
{
    ensureOwner();
    // Retrieve from map
    auto iter = m_meshes.find(id);
    if (iter == m_meshes.end())
    {
        return false;
    }
    // Copy data
    vertices = iter->second.m_vertices;
    indices = iter->second.m_indices;
    normals = iter->second.m_normals;
    uvs = iter->second.m_uvs;
    type = iter->second.m_type;
    return true;
}

ResourceId CResourceManager::createImage(const std::vector<unsigned char>& imageData,
                                         unsigned int width, unsigned int height,
                                         EColorFormat format)
{
    ensureOwner();
    // Create image
    ResourceId id = m_nextImageId;
    ++m_nextImageId;

    // TODO Sanity check if mesh id exists?
    // Add mesh
    m_images[id] = SImage(imageData, width, height, format);

    // Notify listener with create event
    notifyResourceListeners(EResourceType::Image, id, EListenerEvent::Create);
    return id;
}

ResourceId CResourceManager::loadImage(const std::string& file, EColorFormat format)
{
    ensureOwner();
    const auto imageKey = file + "#" + std::to_string(static_cast<int>(format));
    auto entry = m_imageFiles.find(imageKey);
    if (entry != m_imageFiles.end())
    {
        return entry->second;
    }
    requireCacheHit(file);

	// Load image
	SImage image;
	if (!load(file, format, image))
	{
		SPDLOG_ERROR("Failed to load image from file {}.", file.c_str());
		return invalidResource;
	}

	// Create managed resource
    ResourceId imageId = createImage(image.m_data, image.m_width, image.m_height, image.m_format);
    if (imageId == invalidResource)
    {
        SPDLOG_ERROR("Failed to create image resource id from file {}.", file.c_str());
        return invalidResource;
    }
    m_imageFiles[imageKey] = imageId;
    return imageId;
}

bool CResourceManager::getImage(ResourceId id, std::vector<unsigned char>& data,
                                unsigned int& width, unsigned int& height,
                                EColorFormat& format) const
{
    ensureOwner();
    // Retrieve from map
    auto iter = m_images.find(id);
    if (iter == m_images.end())
    {
        return false;
    }
    // Copy data
    data = iter->second.m_data;
    width = iter->second.m_width;
    height = iter->second.m_height;
    format = iter->second.m_format;
    return true;
}

ResourceId CResourceManager::createMaterial(ResourceId base, ResourceId normal,
                                            ResourceId specular, ResourceId glow, ResourceId alpha)
{
    ensureOwner();
    // Create material
    ResourceId id = m_nextMaterialId;
    ++m_nextMaterialId;

    // Add material
    m_materials[id] = SMaterial(base, normal, specular, glow, alpha);

    // Notify listener with create event
    notifyResourceListeners(EResourceType::Material, id, EListenerEvent::Create);
    return id;
}

ResourceId CResourceManager::loadMaterial(const std::string& file)
{
    ensureOwner();
    auto entry = m_materialFiles.find(file);
    if (entry != m_materialFiles.end())
    {
        return entry->second;
    }
    requireCacheHit(file);

	SPDLOG_DEBUG("Loading material from file {}.", file.c_str());
	SMaterial material;
	if (!load(file, *this, material))
	{
		SPDLOG_ERROR("Failed to load material from file {}.", file.c_str());
		return invalidResource;
	}

    ResourceId materialId =
		createMaterial(material.m_base, material.m_normal, material.m_specular, material.m_glow, material.m_alpha);
    if (materialId == invalidResource)
    {
        SPDLOG_ERROR("Failed to create material resource id for material file {}.", file.c_str());
        return invalidResource;
    }
    setMaterialSettings(materialId, material.settings);
    m_materialFiles[file] = materialId;
    return materialId;
}

bool CResourceManager::getMaterial(ResourceId id, ResourceId& base, ResourceId& normal,
                                   ResourceId& specular, ResourceId& glow, ResourceId& alpha) const
{
    ensureOwner();
    // Retrieve from map
    auto iter = m_materials.find(id);
    if (iter == m_materials.end())
    {
        return false;
    }
    // Copy data
    base = iter->second.m_base;
    normal = iter->second.m_normal;
    specular = iter->second.m_specular;
    glow = iter->second.m_glow;
    alpha = iter->second.m_alpha;
    return true;
}

ResourceId CResourceManager::createModel(ResourceId mesh, ResourceId material)
{
    ensureOwner();
	// Create model
	ResourceId id = m_nextModelId;
	++m_nextModelId;

	// Add material
	m_models[id] = SModel(mesh, material);

	// Notify listener with create event
	notifyResourceListeners(EResourceType::Model, id, EListenerEvent::Create);
	return id;
}

ResourceId CResourceManager::loadModel(const std::string& file)
{
    ensureOwner();
	auto entry = m_modelFiles.find(file);
	if (entry != m_modelFiles.end())
	{
		return entry->second;
	}
    requireCacheHit(file);

	SPDLOG_DEBUG("Loading model from file {}.", file.c_str());
	SModel model;
	if (!load(file, *this, model))
	{
		SPDLOG_ERROR("Failed to load model from file {}.", file.c_str());
		return invalidResource;
	}

	ResourceId modelId = createModel(model.m_mesh, model.m_material);
	if (modelId == invalidResource)
	{
		SPDLOG_ERROR("Failed to create model resource id for model file {}.", file.c_str());
		return invalidResource;
	}
	m_modelFiles[file] = modelId;
	return modelId;
}

bool CResourceManager::getModel(ResourceId id, ResourceId& mesh, ResourceId& material)
{
    ensureOwner();
	// Retrieve from map
	auto iter = m_models.find(id);
	if (iter == m_models.end())
	{
		return false;
	}
	// Copy data
	mesh = iter->second.m_mesh;
	material = iter->second.m_material;
	return true;
}

ResourceId CResourceManager::createString(const std::string& text)
{
    ensureOwner();
    // Create string
    ResourceId id = m_nextStringId;
    ++m_nextStringId;

    // Add string
    m_strings[id] = text;

    // Notify listener with create event
    notifyResourceListeners(EResourceType::String, id, EListenerEvent::Create);
    return id;
}

ResourceId CResourceManager::loadString(const std::string& file)
{
    ensureOwner();
	return loadString(file, false);
}

bool CResourceManager::getString(ResourceId id, std::string& text) const
{
    ensureOwner();
    // Retrieve from map
    auto iter = m_strings.find(id);
    if (iter == m_strings.end())
    {
        return false;
    }
    // Copy data
    text = iter->second;
    return true;
}

ResourceId CResourceManager::createShader(ResourceId vertex, ResourceId tessCtrl,
                                          ResourceId tessEval, ResourceId geometry,
                                          ResourceId fragment)
{
    ensureOwner();
    // Create shader
    ResourceId id = m_nextShaderId;
    ++m_nextShaderId;

    // Add shader
    m_shaders[id] = SShader(vertex, tessCtrl, tessEval, geometry, fragment);

    // Notify listener with create event
    notifyResourceListeners(EResourceType::Shader, id, EListenerEvent::Create);
    return id;
}

bool CResourceManager::getShader(ResourceId id, ResourceId& vertex, ResourceId& tessCtrl,
                                 ResourceId& tessEval, ResourceId& geometry,
                                 ResourceId& fragment) const
{
    ensureOwner();
    // Retrieve from map
    auto iter = m_shaders.find(id);
    if (iter == m_shaders.end())
    {
        return false;
    }
    // Copy data
    vertex = iter->second.m_vertex;
    tessCtrl = iter->second.m_tessCtrl;
    tessEval = iter->second.m_tessEval;
    geometry = iter->second.m_geometry;
    fragment = iter->second.m_fragment;
    return true;
}

ResourceId CResourceManager::loadShader(const std::string& file)
{
    ensureOwner();
    // Check if shader exists
    auto entry = m_shaderFiles.find(file);
    if (entry != m_shaderFiles.end())
    {
        return entry->second;
    }
    requireCacheHit(file);

	SPDLOG_DEBUG("Loading shader from file {}.", file.c_str());
	SShader shader;
	if (!load(file, *this, shader))
	{
		SPDLOG_ERROR("Failed to load shader from file {}.", file.c_str());
		return invalidResource;
	}

	ResourceId shaderId = invalidResource;
	shaderId = createShader(shader.m_vertex, shader.m_tessCtrl, shader.m_tessEval, shader.m_geometry, shader.m_fragment);
	if (shaderId == invalidResource)
	{
		SPDLOG_ERROR("Failed to create reasource id for shader file {}.", file.c_str());
		return invalidResource;
	}

	m_shaderFiles[file] = shaderId;
	return shaderId;
}

void CResourceManager::addResourceListener(IResourceListener* listener)
{
    ensureOwner();
    if (std::find(m_resourceListeners.begin(), m_resourceListeners.end(), listener) != m_resourceListeners.end()) return;
    listener->onAttach(this);
    m_resourceListeners.push_back(listener);
    try {
    // Replay dependencies before their users for late GPU listeners.
    const auto replay = [&](const auto& entries, EResourceType type) { for (const auto& entry : entries) listener->notify(type, entry.first, EListenerEvent::Create, this); };
    replay(m_strings, EResourceType::String);
    replay(m_images, EResourceType::Image);
    replay(m_meshes, EResourceType::Mesh);
    replay(m_materials, EResourceType::Material);
    replay(m_models, EResourceType::Model);
    replay(m_shaders, EResourceType::Shader);
    } catch (...) {
        m_resourceListeners.remove(listener);
        listener->onDetach(this);
        throw;
    }
}

void CResourceManager::removeResourceListener(IResourceListener* listener)
{
    ensureOwner();
    m_resourceListeners.remove(listener);
    listener->onDetach(this);
}

void CResourceManager::notifyResourceListeners(EResourceType type, ResourceId id,
                                               EListenerEvent event)
{
    ensureOwner();
    if(m_onPrepared) m_onPrepared();
    for (const auto& listener : m_resourceListeners)
    {
        listener->notify(type, id, event, this);
    }
}

ResourceId CResourceManager::loadString(const std::string& file, bool preprocess)
{
    ensureOwner();
	const auto cacheKey = file + (preprocess ? "|shader" : "|text");
    auto iter = m_textFiles.find(cacheKey);
	if (iter != m_textFiles.end())
	{
		return iter->second;
	}
    requireCacheHit(file);

	SPDLOG_DEBUG("Loading text from file {}.", file.c_str());
	std::ifstream ifs(file);
	if (!ifs.is_open())
	{
		SPDLOG_ERROR("Failed to open the text file {}.", file.c_str());
		return -1;
	}

	// Load file
	std::string text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();

	ResourceId stringId = -1;

	if (preprocess)
	{
		// Preprocessing of include statements for shader source files
		CShaderPreprocessor preprocessor;
		preprocessor.setIncludePath("data/shader/include/");
		if (!preprocessor.preprocess(text, text))
		{
			SPDLOG_ERROR("Failed to preprocess the text file {}.", file.c_str());
			return -1;
		}
	}

	// Create new string entry
	stringId = createString(text);
	if (stringId == -1)
	{
		SPDLOG_ERROR("Failed to create string id for text file {}.", file.c_str());
		return -1;
	}

	m_textFiles[cacheKey] = stringId;
	return stringId;
}
SMaterialSettings CResourceManager::getMaterialSettings(ResourceId id) const
{
    ensureOwner();
    auto it = m_materials.find(id);
    return it == m_materials.end() ? SMaterialSettings{} : it->second.settings;
}
bool CResourceManager::setMaterialSettings(ResourceId id, const SMaterialSettings& settings)
{
    ensureOwner();
    auto it = m_materials.find(id);
    if (it == m_materials.end() || !std::isfinite(settings.opacity) || !std::isfinite(settings.alphaCutoff) || settings.opacity < 0.f || settings.opacity > 1.f || settings.alphaCutoff < 0.f || settings.alphaCutoff > 1.f) return false;
    it->second.settings = settings;
    notifyResourceListeners(EResourceType::Material, id, EListenerEvent::Change);
    return true;
}
bool CResourceManager::updateString(ResourceId id, const std::string& text)
{
    ensureOwner();
    if (!m_strings.contains(id)) return false;
    m_strings[id] = text;
    notifyResourceListeners(EResourceType::String, id, EListenerEvent::Change);
    for (const auto& [shaderId, shader] : m_shaders)
        if (shader.m_vertex == id || shader.m_fragment == id || shader.m_geometry == id || shader.m_tessCtrl == id || shader.m_tessEval == id)
            notifyResourceListeners(EResourceType::Shader, shaderId, EListenerEvent::Change);
    return true;
}
bool CResourceManager::reloadShader(ResourceId id)
{
    ensureOwner();
    if(m_cacheOnly) throw std::logic_error("Synchronous shader reload is disabled during rendering");
    if (!m_shaders.contains(id)) return false;
    const auto& shader = m_shaders.at(id);
    std::unordered_map<ResourceId, std::string> staged;
    for (auto stage : {shader.m_vertex, shader.m_fragment, shader.m_geometry, shader.m_tessCtrl, shader.m_tessEval})
    {
        if (stage == invalidResource) continue;
        for (const auto& [file, stringId] : m_textFiles)
        {
            if (stringId != stage) continue;
            const auto path = file.substr(0,file.rfind('|'));
            std::ifstream stream(path);
            if (!stream) return false;
            std::string code((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            CShaderPreprocessor preprocessor;
            preprocessor.setIncludePath("data/shader/include/");
            std::string expanded;
            if (!preprocessor.preprocess(code, expanded)) return false;
            staged[stage] = std::move(expanded);
        }
    }
    if (staged.empty()) return false;
    for (const auto& [stage, code] : staged) m_strings[stage] = code;
    for (const auto& [shaderId, program] : m_shaders)
        if (staged.contains(program.m_vertex) || staged.contains(program.m_fragment) || staged.contains(program.m_geometry) || staged.contains(program.m_tessCtrl) || staged.contains(program.m_tessEval))
            notifyResourceListeners(EResourceType::Shader, shaderId, EListenerEvent::Change);
    return true;
}
bool CResourceManager::removeResource(EResourceType type, ResourceId id)
{
    ensureOwner();
    // Refuse dangling dependencies. Scene handles are caller-owned and must be released first.
    for (const auto& [unused, material] : m_materials)
        if (type == EResourceType::Image && (material.m_base == id || material.m_normal == id || material.m_specular == id || material.m_glow == id || material.m_alpha == id)) return false;
    for (const auto& [unused, model] : m_models)
        if ((type == EResourceType::Mesh && model.m_mesh == id) || (type == EResourceType::Material && model.m_material == id)) return false;
    for (const auto& [unused, shader] : m_shaders)
        if (type == EResourceType::String && (shader.m_vertex == id || shader.m_fragment == id || shader.m_geometry == id || shader.m_tessCtrl == id || shader.m_tessEval == id)) return false;
    const auto erase = [&](auto& resources, auto& files) {
        if (!resources.contains(id)) return false;
        notifyResourceListeners(type, id, EListenerEvent::Delete);
        resources.erase(id);
        std::erase_if(files, [id](const auto& entry) { return entry.second == id; });
        return true;
    };
    switch (type)
    {
    case EResourceType::Mesh: return erase(m_meshes, m_meshFiles);
    case EResourceType::Image: return erase(m_images, m_imageFiles);
    case EResourceType::Material: return erase(m_materials, m_materialFiles);
    case EResourceType::Model: return erase(m_models, m_modelFiles);
    case EResourceType::String: return erase(m_strings, m_textFiles);
    case EResourceType::Shader: return erase(m_shaders, m_shaderFiles);
    }
    return false;
}

bool CResourceManager::reloadResource(EResourceType type, ResourceId id)
{
    ensureOwner();
    if(m_cacheOnly) throw std::logic_error("Synchronous resource reload is disabled during rendering");
    const auto pathFor=[id](const auto& files) {for(const auto& [path,value]:files) if(value==id) return path; return std::string{};};
    switch(type) {
    case EResourceType::Shader: return reloadShader(id);
    case EResourceType::Mesh: {
        auto path=pathFor(m_meshFiles);SMesh value;if(path.empty()||!load(path,value)) return false;
        m_meshes.at(id)=std::move(value);break;
    }
    case EResourceType::Image: {
        auto key=pathFor(m_imageFiles);if(key.empty()||!m_images.contains(id)) return false;
        SImage value;if(!load(key.substr(0,key.rfind('#')),m_images.at(id).m_format,value)) return false;
        m_images.at(id)=std::move(value);break;
    }
    case EResourceType::Material: {
        auto path=pathFor(m_materialFiles);SMaterial value;if(path.empty()||!load(path,*this,value)) return false;
        m_materials.at(id)=std::move(value);break;
    }
    case EResourceType::Model: {
        auto path=pathFor(m_modelFiles);SModel value;if(path.empty()||!load(path,*this,value)) return false;
        m_models.at(id)=std::move(value);break;
    }
    case EResourceType::String: {
        auto key=pathFor(m_textFiles);if(key.empty()) return false;
        std::ifstream stream(key.substr(0,key.rfind('|')));if(!stream) return false;
        std::string text((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());
        if(key.ends_with("|shader")) {CShaderPreprocessor p;p.setIncludePath("data/shader/include/");if(!p.preprocess(text,text)) return false;}
        return updateString(id,text);
    }
    }
    notifyResourceListeners(type,id,EListenerEvent::Change);
    return true;
}
