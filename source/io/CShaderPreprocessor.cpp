#include "CShaderPreprocessor.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>

void CShaderPreprocessor::setIncludePath(const std::string& path) { m_includePath=path; }
bool CShaderPreprocessor::preprocess(const std::string& source,std::string& output)
{
    std::set<std::string> stack;
    std::function<bool(const std::string&,std::ostringstream&,unsigned)> expand;
    expand=[&](const std::string& code,std::ostringstream& result,unsigned depth) {
        if(depth>16) {SPDLOG_ERROR("Shader include nesting exceeded 16 levels.");return false;}
        std::istringstream lines(code);std::string line;unsigned number=0;
        while(std::getline(lines,line)) {
            ++number;auto start=line.find_first_not_of(" \t");
            if(start!=std::string::npos && line.compare(start,8,"#include")==0) {
                auto first=line.find('"',start+8),last=line.find('"',first==std::string::npos?line.size():first+1);
                if(first==std::string::npos || last==std::string::npos) return false;
                const auto relative=std::filesystem::path(line.substr(first+1,last-first-1)).lexically_normal();
                if(relative.has_root_path()) return false;
                for(const auto& component:relative) if(component=="..") return false;
                auto path=(std::filesystem::path(m_includePath)/relative).lexically_normal().generic_string();
                if(!stack.insert(path).second) {SPDLOG_ERROR("Cyclic shader include: {}",path.c_str());return false;}
                std::ifstream file(path);if(!file) {SPDLOG_ERROR("Missing shader include: {}",path.c_str());return false;}
                std::string included((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
                result<<"\n#line 1\n";
                if(!expand(included,result,depth+1)) return false;
                result<<"\n#line "<<number+1<<'\n'; stack.erase(path);
            } else result<<line<<'\n';
        }
        return true;
    };
    std::ostringstream result;
    if(!expand(source,result,0)) return false;
    output=result.str();
    return true;
}
