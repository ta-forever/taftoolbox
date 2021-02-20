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

        std::string getValue(const std::string& key, const std::string &default) const;
        const TdfFile& getChild(const std::string& key) const;

        void serialise(std::ostream& os) const;
        void deserialise(std::istream& is);

        void dumpjson(std::ostream& os, int indent=0) const;

    private:
        std::size_t tdfParse(const std::string& text, std::size_t pos, int maxDepth);
    };

}
