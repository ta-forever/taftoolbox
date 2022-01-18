#pragma once

#include <map>
#include <string>

namespace ta
{

    struct ci_less
    {
        // case-independent (ci) compare_less binary function
        struct nocase_compare
        {
            bool operator() (const unsigned char& c1, const unsigned char& c2) const {
                return tolower(c1) < tolower(c2);
            }
        };
        bool operator() (const std::string& s1, const std::string& s2) const {
            return std::lexicographical_compare
            (s1.begin(), s1.end(),   // source range
                s2.begin(), s2.end(),   // dest range
                nocase_compare());  // comparison
        }
    };

    class TdfFile
    {
    public:
        TdfFile();
        TdfFile(const std::string &text, int maxDepth);

        std::map<std::string, std::string, ci_less> values;
        std::map<std::string, TdfFile, ci_less> children;

        std::string getValue(const std::string& key, const std::string &def) const;
        std::string getValueOriginalCase(const std::string& key, const std::string& def) const;
        const TdfFile& getChild(const std::string& key) const;

        void serialise(std::ostream& os) const;
        void deserialise(std::istream& is);

        void dumpjson(std::ostream& os, int indent=0) const;

    private:
        std::size_t tdfParse(const std::string& text, std::size_t pos, int maxDepth);
    };

}
