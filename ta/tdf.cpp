#include "tdf.h"
#include <sstream>
#include <regex>
#include <cstdint>

using namespace ta;

static std::string toLower(const std::string& s)
{
    std::string lower;
    std::transform(s.begin(), s.end(), std::back_inserter(lower), ::tolower);
    return lower;
}

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
            result += '\n';
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
    static std::regex e("^\\s+|\\s+$");   // remove leading and trailing spaces. NB regex compilation very slow on mingw, so keep it static!
    return std::regex_replace(s, e, "");
}

TdfFile::TdfFile()
{ }

TdfFile::TdfFile(const std::string &_text, int maxDepth)
{
    std::string text = stripComments(_text);
    std::size_t pos = 0u;
    do
    {
        pos = tdfParse(text, pos, maxDepth);
    }
    while (pos < text.size());
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
            //std::cout << "<empty line>" << std::endl;
            continue;
        }
        //transform(line.begin(), line.end(), line.begin(), ::tolower);
        //std::cout << line << std::endl;

        std::size_t posEquals = line.find_first_of('=');
        if (line.size()>2 && line.front() == '[' && line.back() == ']')
        {
            if (maxDepth > 0)
            {
                std::string subHeading = line.substr(1, line.size() - 2);
                //std::cout << "  <header>" << subHeading << std::endl;
                pos = this->children[subHeading].tdfParse(text, pos, maxDepth - 1);
                //std::cout << "  <header>" << subHeading << " resume pos=" << pos << std::endl;
            }
            else
            {
                //std::cout << "  <header> maxDepth" << std::endl;
                return text.size();
            }
        }
        else if (line == "{")
        {
            ++braceDepth;
            //std::cout << "  <open brace> braceDepth=" << braceDepth << std::endl;
        }
        else if (line == "}")
        {
            --braceDepth;
            //std::cout << "  <close brace> braceDepth=" << braceDepth << std::endl;
        }
        else if (posEquals != std::string::npos)
        {
            std::string key = trim(line.substr(0, posEquals));
            std::string value = trim(line.substr(posEquals + 1));
            //std::cout << "  <equals> key:" << key << ", val:" << value << std::endl;
            if (value.back() == ';')
            {
                value = value.substr(0, value.size() - 1);
            }
            this->values[key] = value;
        }

        if (braceDepth == 0)
        {
            //std::cout << "  <braceDepth=0> return pos=" << pos << std::endl;
            return pos;
        }
    }
    //std::cout << "  <while break> return pos=" << pos << '/' << text.size() << std::endl;
    return pos;
}

std::string TdfFile::getValue(const std::string& key, const std::string& def) const
{
    auto it = values.find(key);
    if (it == values.end())
    {
        return def;
    }
    else
    {
        return toLower(it->second);
    }
}

std::string TdfFile::getValueOriginalCase(const std::string& key, const std::string& def) const
{
    auto it = values.find(key);
    if (it == values.end())
    {
        return def;
    }
    else
    {
        return it->second;
    }
}


const TdfFile& TdfFile::getChild(const std::string& key) const
{
    static const TdfFile emptyTdf;
    auto it = children.find(key);
    if (it == children.end())
    {
        return emptyTdf;
    }
    else
    {
        return it->second;
    }
}

static void writeInt(std::ostream& os, std::uint32_t i)
{
    os.write((const char*)&i, sizeof(i));
}

static void writeString(std::ostream& os, const std::string& s)
{
    writeInt(os, s.size());
    os.write(s.data(), s.size());
}

void TdfFile::serialise(std::ostream& os) const
{
    writeInt(os, values.size());
    for (const auto& p : values)
    {
        writeString(os, p.first);
        writeString(os, p.second);
    }
    writeInt(os, children.size());
    for (const auto& p : children)
    {
        writeString(os, p.first);
        p.second.serialise(os);
    }
}

static std::uint32_t readInt(std::istream& is)
{
    std::uint32_t i;
    is.read((char*)&i, sizeof(i));
    return i;
}

static std::string readString(std::istream& is)
{
    std::string s;
    s.resize(readInt(is));
    is.read(&s[0], s.size());
    return s;
}

void TdfFile::deserialise(std::istream& is)
{
    std::uint32_t nValues = readInt(is);
    for (std::uint32_t n = 0u; n < nValues; ++n)
    {
        std::string key = readString(is);
        values[key] = readString(is);
    }
    std::uint32_t nChilds = readInt(is);
    for (std::uint32_t n = 0u; n < nChilds; ++n)
    {
        std::string name = readString(is);
        children[name].deserialise(is);
    }
}

void TdfFile::dumpjson(std::ostream& os, int indent) const
{
    for (int i = 0; i < indent; ++i) os << ' ';
    os << "{\n";
    indent += 2;

    int nVal = 0;
    for (const auto& val : values)
    {
        for (int i = 0; i < indent; ++i) os << ' ';
        os << '"' << val.first << "\":\"" << val.second << '"';

        ++nVal;
        if (nVal < values.size() || children.size()>0)
            os << ",\n";
        else
            os << "\n";
    }

    for (const auto& child : children)
    {
        for (int i = 0; i < indent; ++i) os << ' ';
        os << '"' << child.first << "\":\n";
        child.second.dumpjson(os, indent);
    }

    indent -= 2;
    for (int i = 0; i < indent; ++i) os << ' ';
    os << "}\n";
}
