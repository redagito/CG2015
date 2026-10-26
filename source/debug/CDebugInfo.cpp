#include "CDebugInfo.h"

void CDebugInfo::setLog(std::vector<std::string> lines)
{
    m_logBuffer=std::move(lines);
}

const std::vector<std::string>& CDebugInfo::getLog() const { return m_logBuffer; }

size_t CDebugInfo::getLogBufferSize() const { return 10; }

void CDebugInfo::setValue(const std::string& key, const std::string& value)
{
    m_values[key] = value;
}

const std::map<std::string, std::string>& CDebugInfo::getValues() const
{
    return m_values;
}
