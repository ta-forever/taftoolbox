#include "tdf.h"
#include <sstream>
#include <regex>

using namespace ta;

static std::string stripComments(const std::string &text)
{
    std::string result;
    result.reserve(text.size());

    bool isComment = false;
    for (std::size_t i = 0u; i < text.size(); ++i)
    {
        if (isComment && text[i] == '\n')
        {
            isComment = false;
        }
        else if (!isComment && text[i] == '/' && i+1<text.size() && text[i+1] == '/')
        {
            isComment = true;
        }
        else if (!isComment)
        {
            result += text[i];
        }
    }

    return result;
}

static std::string trim(const std::string &s) {
    std::regex e("^\\s+|\\s+$");   // remove leading and trailing spaces
    return std::regex_replace(s, e, "");
}

TdfFile::TdfFile()
{ }

TdfFile::TdfFile(const std::string &text, int maxDepth)
{
    tdfParse(stripComments(text), 0u, maxDepth);
}

std::size_t TdfFile::tdfParse(const std::string &text, std::size_t pos, int maxDepth)
{
    int braceDepth = 0;

    while(pos < text.size())
    {
        std::string line;
        std::size_t posNL = text.find_first_of('\n', pos);
        if (posNL != std::string::npos)
        {
            line = text.substr(pos, posNL - pos);
            pos = 1u+posNL;
        }
        else
        {
            line = text.substr(pos);
            pos = text.size();
        }

        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        std::size_t posEquals = line.find_first_of('=');
        if (line.size()>2 && line.front() == '[' && line.back() == ']')
        {
            if (maxDepth > 0)
            {
                std::string subHeading = line.substr(1, line.size() - 2);
                pos = this->children[subHeading].tdfParse(text, pos, maxDepth - 1);
            }
            else
            {
                return text.size();
            }
        }
        else if (line == "{")
        {
            ++braceDepth;
        }
        else if (line == "}")
        {
            --braceDepth;
        }
        else if (posEquals != std::string::npos)
        {
            std::string key = trim(line.substr(0, posEquals));
            std::string value = trim(line.substr(posEquals + 1));
            if (value.back() == ';')
            {
                value = value.substr(0, value.size() - 1);
            }
            this->values[key] = value;
        }

        if (braceDepth == 0)
        {
            return pos;
        }
    }
    return pos;
}
