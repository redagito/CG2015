#pragma once

#include <cstdint>

/**
* \brief CRC32 implementation.
*/
uint32_t crc32Start();
uint32_t crc32Update(uint32_t currentHash, uint8_t data);
uint32_t crc32Update(uint32_t currentHash, const uint8_t* data, uint32_t size);
uint32_t crc32End(uint32_t currentHash);

/**
* \brief Utility function for CRC32 calculation.
*
* Calculates CRC32 hash for a block of data.
*/
uint32_t crc32Calculate(const uint8_t* data, uint32_t size);