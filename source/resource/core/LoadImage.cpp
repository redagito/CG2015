#include <vector>

#include <lodepng.h>

#include "LoadImage.h"
#include <spdlog/spdlog.h>

bool load(const std::string& file, EColorFormat format, SImage& image)
{
	if (file.find(".png") != std::string::npos)
	{
		return loadPng(file, format, image);
	}
	SPDLOG_ERROR("The image file {} has an invalid or unknown format.", file.c_str());
	return false;
}

bool loadPng(const std::string& file, EColorFormat format, SImage& image)
{
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
		SPDLOG_ERROR("Unknown color format encountered while loading image file {}.", file.c_str());
		return false;
		break;
	}

	// Decode image data
	unsigned int err = lodepng::decode(image.m_data, image.m_width, image.m_height, file, colorType);
	if (err != 0)
	{
		SPDLOG_ERROR("An error occured while decoding the image file {}: {}", file.c_str(), lodepng_error_text(err));
		return false;
	}
	image.m_format = format;
	return true;
}