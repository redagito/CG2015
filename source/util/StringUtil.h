#pragma once

#include <string>
#include <cstdint>

std::string getFileExtension(const std::string& file);
uint32_t crc32Hash(const std::string& text);