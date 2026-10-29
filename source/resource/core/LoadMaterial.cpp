#include "LoadMaterial.h"
#include "util/StringUtil.h"

#include "CResourceManager.h"
#include "io/CIniFile.h"
#include "io/JsonDeserialize.h"
#include "io/JsonUtil.h"
#include <spdlog/spdlog.h>

bool load(const std::string &file, CResourceManager &manager, SMaterial &material)
{
    std::string base;
    std::string normal;
    std::string specular;
    std::string glow;
    std::string alpha;

    if (file.find(".ini") != std::string::npos)
    {
        if (!loadMaterialFromIni(file, base, normal, specular, glow, alpha))
        {
            return false;
        }
    }
    else if (file.find(".json") != std::string::npos)
    {
        if (!loadMaterialFromJson(file, base, normal, specular, glow, alpha))
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    ResourceId baseId = invalidResource;
    if (!base.empty())
    {
        // Diffuse texture is RGB format, ignore alpha
        baseId = manager.loadImage(base, EColorFormat::RGB24);
        if (baseId == invalidResource)
        {
            SPDLOG_ERROR("Failed to load base.");
            return false;
        }
    }

    ResourceId normalId = invalidResource;
    if (!normal.empty())
    {
        // Normal texture is RGB format
        normalId = manager.loadImage(normal, EColorFormat::RGB24);
        if (normalId == invalidResource)
        {
            SPDLOG_ERROR("Failed to load normal texture.");
            return false;
        }
    }

    ResourceId specularId = invalidResource;
    if (!specular.empty())
    {
        // Specular texture is grey-scale format
        specularId = manager.loadImage(specular, EColorFormat::GreyScale8);
        if (specularId == invalidResource)
        {
            SPDLOG_ERROR("Failed to load specular texture.");
            return false;
        }
    }

    ResourceId glowId = invalidResource;
    if (!glow.empty())
    {
        // Glow texture is grey-scale format
        glowId = manager.loadImage(glow, EColorFormat::GreyScale8);
        if (glowId == invalidResource)
        {
            SPDLOG_ERROR("Failed to load glow texture.");
            return false;
        }
    }

    ResourceId alphaId = invalidResource;
    if (!alpha.empty())
    {
        // Alpha texture is grey-scale format
        alphaId = manager.loadImage(alpha, EColorFormat::GreyScale8);
        if (alphaId == invalidResource)
        {
            SPDLOG_ERROR("Failed to load alpha texture.");
            return false;
        }
    }

    // Set ids
    material.m_base = baseId;
    material.m_normal = normalId;
    material.m_specular = specularId;
    material.m_glow = glowId;
    material.m_alpha = alphaId;
    if (alphaId != invalidResource)
        material.settings.alphaMode = AlphaMode::Mask;
    if (getFileExtension(file) == "json")
    {
        Json::Value root;
        if (!load(file, root))
            return false;
        const auto mode =
            root.get("alpha_mode", alphaId == invalidResource ? "opaque" : "mask").asString();
        if (mode == "opaque")
            material.settings.alphaMode = AlphaMode::Opaque;
        else if (mode == "mask")
            material.settings.alphaMode = AlphaMode::Mask;
        else if (mode == "blend")
            material.settings.alphaMode = AlphaMode::Blend;
        else
        {
            SPDLOG_ERROR("Unknown alpha_mode in {}", file.c_str());
            return false;
        }
        material.settings.alphaCutoff = root.get("alpha_cutoff", 0.5).asFloat();
        material.settings.opacity = root.get("opacity", 1.0).asFloat();
        material.settings.srgb = root.get("color_space", "srgb").asString() == "srgb";
        material.settings.background = root.get("background", false).asBool();
        if (root.isMember("emissive_accents") &&
            !material.settings.emissiveAccents.load(root["emissive_accents"]))
        {
            SPDLOG_ERROR("Invalid emissive accents in {}", file);
            return false;
        }
        if (material.settings.alphaCutoff < 0.f || material.settings.alphaCutoff > 1.f ||
            material.settings.opacity < 0.f || material.settings.opacity > 1.f)
            return false;
    }
    return true;
}

bool loadMaterialFromIni(const std::string &file, std::string &base, std::string &normal,
                         std::string &specular, std::string &glow, std::string &alpha)
{
    SPDLOG_DEBUG("Loading material from file {}.", file.c_str());
    CIniFile ini;
    if (!ini.load(file))
    {
        SPDLOG_ERROR("Failed to load material file {} as ini file.", file.c_str());
        return false;
    }

    // Load from ini
    base = ini.getValue("base", "file", "");
    normal = ini.getValue("normal", "file", "");
    specular = ini.getValue("specular", "file", "");
    glow = ini.getValue("glow", "file", "");
    alpha = ini.getValue("alpha", "file", "");
    return true;
}

bool loadMaterialFromJson(const std::string &file, std::string &base, std::string &normal,
                          std::string &specular, std::string &glow, std::string &alpha)
{
    Json::Value root;
    if (!load(file, root))
    {
        SPDLOG_ERROR("Failed to load material file {} as json file.", file.c_str());
        return false;
    }

    // Texture maps are optional; present entries must still be valid strings.
    const auto map = [&root](const char *name, std::string &value)
    {
        value.clear();
        return !root.isMember(name) || deserialize(root[name], value);
    };
    return root.isObject() && map("base", base) && map("normal", normal) &&
           map("specular", specular) && map("glow", glow) && map("alpha", alpha);
}
