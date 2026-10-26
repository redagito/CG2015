#pragma once

#include <vector>
#include <map>
#include <string>


/**
 * \brief Stores debug data.
 * Stores an overlay snapshot and key/value debug data pairs.
 */
class CDebugInfo
{
   public:
    void setLog(std::vector<std::string> lines);

    const std::vector<std::string>& getLog() const;

    size_t getLogBufferSize() const;

    void setValue(const std::string& key, const std::string& value);
    const std::map<std::string, std::string>& getValues() const;

   private:
    std::vector<std::string> m_logBuffer; /**< Main-thread snapshot of the spdlog ring sink. */
    std::map<std::string, std::string> m_values; /**< Stores debug key/value pairs. */
};
