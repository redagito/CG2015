#include "CResourceManager.h"

#include <fstream>
#include <sstream>

#include "lodepng.h"
#include "tiny_obj_loader.h"

#include "resource/IResourceListener.h"

#include "io/JsonUtil.h"
#include "io/JsonDeserializer.h"
#include "io/CIniFile.h"
#include "io/CObjModelLoader.h"
#include "io/CShaderPreprocessor.h"

#include "LoadMaterial.h"
#include "LoadShader.h"

#include "debug/Log.h"

CResourceManager::CResourceManager()
    : m_nextMeshId(0), m_nextImageId(0), m_nextMaterialId(0), m_nextStringId(0), m_nextShaderId(0)
{
    return;
}

ResourceId CResourceManager::createMesh(const std::vector<float>& vertices,
                                        const std::vector<unsigned int>& indices,
                                        const std::vector<float>& normals,
                                        const std::vector<float>& uvs, EPrimitiveType type)
{
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
    auto entry = m_meshFiles.find(file);
    if (entry != m_meshFiles.end())
    {
        return entry->second;
    }

    // Retrieve file extension
    auto pos = file.find_last_of('.');
    if (pos == std::string::npos)
    {
        LOG_ERROR("The mesh file %s does not have a file extension and could not be loaded.",
                  file.c_str());
        return -1;
    }
    std::string extension = file.substr(file.find_last_of('.') + 1);

    // Mesh resource id
    ResourceId meshId = -1;

    // Decide loading method based on extension
    // TODO Register loader functions for extensions
    if (extension == "obj")
    {
        // Wavefront OBJ file format loaded with tinyobj
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        // Load as obj
        std::string err = tinyobj::LoadObj(shapes, materials, file.c_str());

        if (!err.empty())
        {
            LOG_ERROR("%s.", err.c_str());
            return -1;
        }
        if (shapes.size() > 1)
        {
            LOG_WARNING("Multple meshes in obj files not supported.");
        }
        if (shapes.empty())
        {
            LOG_ERROR("No mesh data loaded from file %s.", file.c_str());
            return -1;
        }
        // Create mesh resource
        meshId = createMesh(shapes.at(0).mesh.positions, shapes.at(0).mesh.indices,
                            shapes.at(0).mesh.normals, shapes.at(0).mesh.texcoords,
                            EPrimitiveType::Triangle);
    }
    else if (extension == "oni")
    {
        // Load without building index buffer
        CObjModelLoader objLoader;
        if (!objLoader.load(file))
        {
            LOG_ERROR("Failed to load mesh file %s as non-indexed obj file.", file.c_str());
            return -1;
        }
        meshId = createMesh(objLoader.getVertices(), {}, objLoader.getNormals(), objLoader.getUV(),
                            EPrimitiveType::Triangle);
    }

    if (meshId == -1)
    {
        LOG_ERROR("Failed to create mesh resource id from file %s.", file.c_str());
        return -1;
    }
    m_meshFiles[file] = meshId;
    return meshId;
}

bool CResourceManager::getMesh(ResourceId id, std::vector<float>& vertices,
                               std::vector<unsigned int>& indices, std::vector<float>& normals,
                               std::vector<float>& uvs, EPrimitiveType& type) const
{
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
    auto entry = m_imageFiles.find(file);
    if (entry != m_imageFiles.end())
    {
        return entry->second;
    }

    // TODO Check extension, png format assumed
    std::vector<unsigned char> data;
    unsigned int width;
    unsigned int height;

	// PNG type check
	if (file.find(".png") == std::string::npos)
	{
		LOG_ERROR("Unknown file format encountered while loading image file %s.", file.c_str());
		return invalidResource;
	}

    // Map color type
    LodePNGColorType colorType;
    switch (format)
    {
    case EColorFormat::GreyScale8:
        colorType = LCT_GREY;
        break;
    case EColorFormat::RGB24:
        colorType = LCT_RGB;
        break;
    case EColorFormat::RGBA32:
        colorType = LCT_RGBA;
        break;
    default:
        LOG_ERROR("Unknown color format encountered while loading image file %s.", file.c_str());
        return -1;
        break;
    }

    // Decode image data
    unsigned int err = lodepng::decode(data, width, height, file, colorType);
    if (err != 0)
    {
        LOG_ERROR("An error occured while decoding the image file %s: %s", file.c_str(),
                  lodepng_error_text(err));
        return -1;
    }

    ResourceId imageId = createImage(data, width, height, format);
    if (imageId == -1)
    {
        LOG_ERROR("Failed to create image resource id from file %s.", file.c_str());
        return -1;
    }
    m_imageFiles[file] = imageId;
    return imageId;
}

bool CResourceManager::getImage(ResourceId id, std::vector<unsigned char>& data,
                                unsigned int& width, unsigned int& height,
                                EColorFormat& format) const
{
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
    auto entry = m_materialFiles.find(file);
    if (entry != m_materialFiles.end())
    {
        return entry->second;
    }

	LOG_DEBUG("Loading material from file %s.", file.c_str());
	SMaterial material;
	if (!load(file, *this, material))
	{
		LOG_ERROR("Failed to load material from file %s.", file.c_str());
		return invalidResource;
	}

    ResourceId materialId =
		createMaterial(material.m_base, material.m_normal, material.m_specular, material.m_glow, material.m_alpha);
    if (materialId == invalidResource)
    {
        LOG_ERROR("Failed to create material resource id for material file %s.", file.c_str());
        return invalidResource;
    }
    m_materialFiles[file] = materialId;
    return materialId;
}

bool CResourceManager::getMaterial(ResourceId id, ResourceId& base, ResourceId& normal,
                                   ResourceId& specular, ResourceId& glow, ResourceId& alpha) const
{
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

ResourceId CResourceManager::createString(const std::string& text)
{
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
	return loadString(file, false);
}

bool CResourceManager::getString(ResourceId id, std::string& text) const
{
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
    // Check if shader exists
    auto entry = m_shaderFiles.find(file);
    if (entry != m_shaderFiles.end())
    {
        return entry->second;
    }

	LOG_DEBUG("Loading shader from file %s.", file.c_str());
	SShader shader;
	if (!load(file, *this, shader))
	{
		LOG_ERROR("Failed to load shader from file %s.", file.c_str());
		return invalidResource;
	}

	ResourceId shaderId = invalidResource;
	shaderId = createShader(shader.m_vertex, shader.m_tessCtrl, shader.m_tessEval, shader.m_geometry, shader.m_fragment);
	if (shaderId == invalidResource)
	{
		LOG_ERROR("Failed to create reasource id for shader file %s.", file.c_str());
		return invalidResource;
	}

	m_shaderFiles[file] = shaderId;
	return shaderId;
}

void CResourceManager::addResourceListener(IResourceListener* listener)
{
    m_resourceListeners.push_back(listener);
    listener->onAttach(this);
}

void CResourceManager::removeResourceListener(IResourceListener* listener)
{
    m_resourceListeners.remove(listener);
    listener->onDetach(this);
}

void CResourceManager::notifyResourceListeners(EResourceType type, ResourceId id,
                                               EListenerEvent event)
{
    for (const auto& listener : m_resourceListeners)
    {
        listener->notify(type, id, event, this);
    }
}

ResourceId CResourceManager::loadString(const std::string& file, bool preprocess)
{
	auto iter = m_textFiles.find(file);
	if (iter != m_textFiles.end())
	{
		return iter->second;
	}

	LOG_DEBUG("Loading text from file %s.", file.c_str());
	std::ifstream ifs(file);
	if (!ifs.is_open())
	{
		LOG_ERROR("Failed to open the text file %s.", file.c_str());
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
			LOG_ERROR("Failed to preprocess the text file %s.", file.c_str());
			return -1;
		}
	}

	// Create new string entry
	stringId = createString(text);
	if (stringId == -1)
	{
		LOG_ERROR("Failed to create string id for text file %s.", file.c_str());
		return -1;
	}

	m_textFiles[file] = stringId;
	return stringId;
}