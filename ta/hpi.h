#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <Windows.h>

namespace ta
{
    class HpiArchive;

    void init();

    class HpiEntry
    {
    public:
        std::string fileName;
        std::string hpiArchive;
        bool isDirectory = false;
        int size = 0;

        std::string load(std::shared_ptr<HpiArchive> &hpi);
    };

    class HpiArchive
    {
        void *m_handle;
        std::string m_hpiPath;

    public:
        HpiArchive(const std::string &path);
        ~HpiArchive();

        std::string name() const;

        void directory(
            std::map<std::string, HpiEntry> &entries,
            const std::string &dirName, 
            std::function<bool(const char*, bool)> match); // filename, isDirectory

        std::string load(const std::string &fileName, int offset, int byteCount);

        static void directory(
            std::map<std::string, HpiEntry> &entries,
            const std::string &gamePath, const std::string &hpiGlobSpec, const std::string &hpiSubDir,
            std::function<bool(const char*, bool)> match); // filename, isDirectory
    };

}
