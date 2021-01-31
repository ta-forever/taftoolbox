#pragma once

#include <map>
#include <string>

namespace ta
{

    class TdfFile
    {
    public:
        TdfFile();
        TdfFile(const std::string &text, int maxDepth);

        std::map<std::string, std::string> values;
        std::map<std::string, TdfFile> children;

    private:
        std::size_t tdfParse(const std::string &text, std::size_t pos, int maxDepth);
    };

}
