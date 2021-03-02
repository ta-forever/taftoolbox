#include <algorithm>
#include <set>
#include <string>
#include <fstream>
#include <iostream>

#include "hpi.h"

using namespace ta;

typedef void (WINAPI *TGetTADirectory) (LPSTR TADir);
typedef LPVOID(WINAPI *THPIOpen) (LPCSTR FileName);
typedef LRESULT(WINAPI *THPIGetFiles)(void *hpi, int Next, LPSTR Name, LPINT Type, LPINT Size);
typedef LRESULT(WINAPI *THPIDir) (void *hpi, int Next, LPCSTR DirName, LPSTR Name, LPINT Type, LPINT Size);
typedef LRESULT(WINAPI *THPIClose) (void *hpi);
typedef LPSTR(WINAPI *THPIOpenFile) (void *hpi, LPCSTR FileName);
typedef void (WINAPI *THPIGet) (void *Dest, void *FileHandle, int offset, int bytecount);
typedef LRESULT(WINAPI *THPICloseFile) (LPSTR FileHandle);
typedef LRESULT(WINAPI *THPIExtractFile) (void *hpi, LPSTR FileName, LPSTR ExtractName);
typedef LPVOID(WINAPI *THPICreate) (LPSTR FileName, void* Callback);
typedef LRESULT(WINAPI *THPICreateDirectory)(void *Pack, LPSTR DirName);
typedef LRESULT(WINAPI *THPIAddFile) (void *Pack, LPSTR HPIName, LPSTR FileName);
typedef LRESULT(WINAPI *THPIAddFileFromMemory) (void *Pack, LPSTR HPIName, LPSTR FileBlock, int fsize);
typedef LRESULT(WINAPI *THPIPackArchive) (void *Pack, int CMethod);
typedef LRESULT(WINAPI *THPIPackFile) (void *Pack);

static bool VERBOSE = false;
static HMODULE dll = 0;
static TGetTADirectory GetTADirectory = 0;
static THPIOpen HPIOpen = 0;
static THPIGetFiles HPIGetFiles = 0;
static THPIDir HPIDir = 0;
static THPIClose HPIClose = 0;
static THPIOpenFile HPIOpenFile = 0;
static THPIGet HPIGet = 0;
static THPICloseFile HPICloseFile = 0;
static THPIExtractFile HPIExtractFile = 0;
static THPICreate HPICreate = 0;
static THPICreateDirectory HPICreateDirectory = 0;
static THPIAddFile HPIAddFile = 0;
static THPIAddFileFromMemory HPIAddFileFromMemory = 0;
static THPIPackArchive HPIPackArchive = 0;
static THPIPackFile HPIPackFile = 0;

void ta::init(bool verbose)
{
    VERBOSE = verbose;
    if (VERBOSE)
    {
        std::cout << "[ta::init]" << std::endl;
    }

    if (dll == 0)
    {
        dll = LoadLibrary("hpiutil.dll");
        if (dll == 0)
        {
            throw std::runtime_error("[hpi::init] Unable to load hpiutil.dll");
        }

        GetTADirectory = (TGetTADirectory)GetProcAddress(dll, "GetTADirectory");
        HPIOpen = (THPIOpen)GetProcAddress(dll, "HPIOpen");
        HPIGetFiles = (THPIGetFiles)GetProcAddress(dll, "HPIGetFiles");
        HPIDir = (THPIDir)GetProcAddress(dll, "HPIDir");
        HPIClose = (THPIClose)GetProcAddress(dll, "HPIClose");
        HPIOpenFile = (THPIOpenFile)GetProcAddress(dll, "HPIOpenFile");
        HPIGet = (THPIGet)GetProcAddress(dll, "HPIGet");
        HPICloseFile = (THPICloseFile)GetProcAddress(dll, "HPICloseFile");
        HPIExtractFile = (THPIExtractFile)GetProcAddress(dll, "HPIExtractFile");
        HPICreate = (THPICreate)GetProcAddress(dll, "HPICreate");
        HPICreateDirectory = (THPICreateDirectory)GetProcAddress(dll, "HPICreateDirectory");
        HPIAddFile = (THPIAddFile)GetProcAddress(dll, "HPIAddFile");
        HPIAddFileFromMemory = (THPIAddFileFromMemory)GetProcAddress(dll, "HPIAddFileFromMemory");
        HPIPackArchive = (THPIPackArchive)GetProcAddress(dll, "HPIPackArchive");
        HPIPackFile = (THPIPackFile)GetProcAddress(dll, "HPIPackFile");

        if (!GetTADirectory || !HPIOpen || !HPIGetFiles || !HPIDir || !HPIClose || !HPIOpenFile ||
            !HPIGet || !HPICloseFile || !HPIExtractFile || !HPICreate || !HPICreateDirectory ||
            !HPIAddFile || !HPIAddFileFromMemory || !HPIPackArchive || !HPIPackFile)
        {
            FreeLibrary(dll);
            dll = 0;
            throw std::runtime_error("hpi::init] DLL load failure");
        }
    }
}



class IFile
{
    LPSTR m_handle;

public:
    IFile(LPSTR handle);
    ~IFile();

    void get(void *dest, int offset, int bytecount);
};


IFile::IFile(LPSTR handle) :
m_handle(handle)
{
    if (m_handle == 0)
    {
        throw std::runtime_error("file not found");
    }
}

IFile::~IFile()
{
    if (m_handle)
    {
        HPICloseFile(m_handle);
    }
}


std::string HpiEntry::load(std::shared_ptr<HpiArchive> &hpi)
{
    if (!hpi || hpi->name() != this->hpiArchive)
    {
        hpi.reset(new HpiArchive(this->hpiArchive));
    }
    return hpi->load(this->fileName, 0, this->size);
}


HpiArchive::HpiArchive(const std::string &path):
m_handle(HPIOpen(path.c_str())),
m_hpiPath(path)
{
    if (VERBOSE)
    {
        std::cout << "[HpiArchive::HpiArchive] path=" << path << std::endl;
    }
    if (m_handle == 0)
    {
        throw std::runtime_error("[HpiArchive::HpiArchive] unable to open archive at path " + path);
    }
}

HpiArchive::~HpiArchive()
{
    if (VERBOSE)
    {
        std::cout << "[HpiArchive::~HpiArchive]" << std::endl;
    }
    if (m_handle)
    {
        HPIClose(m_handle);
    }
}

std::string HpiArchive::name() const
{
    return m_hpiPath;
}

void HpiArchive::directory(std::map<std::string, HpiEntry> &entries, const std::string &dirName, std::function<bool(const char*, bool)> match)
{
    if (VERBOSE)
    {
        std::cout << "[HpiArchive::directory] dirName=" << dirName << std::endl;
    }

    char fileName[MAX_PATH];
    int fileType, size;
    std::function<LRESULT(int n)> next;

    if (dirName.empty())
    {
        next = [this, &fileName, &fileType, &size](int n) {
            return HPIGetFiles(this->m_handle, n, fileName, &fileType, &size);
        };
    }
    else
    {
        next = [this, &fileName, &dirName, &fileType, &size](int n) {
            return HPIDir(this->m_handle, n, dirName.c_str(), fileName, &fileType, &size);
        };
    }

    for (int n = 0; next(n); ++n)
    {
        bool isDirectory = fileType == 1;
        if (match(fileName, isDirectory))
        {
            std::string path = dirName.empty()
                ? fileName
                : dirName + "\\" + fileName;
            HpiEntry &entry = entries[path];
            entry.fileName = path;
            entry.hpiArchive = this->name();
            entry.isDirectory = isDirectory;
            entry.size = size;
        }

        if (isDirectory)
        {
            directory(entries, dirName + "\\" + fileName, match);
        }
    }
}

std::string HpiArchive::load(const std::string &fileName, int offset, int byteCount)
{
    if (VERBOSE)
    {
        std::cout << "[HpiArchive::load] fileName=" << fileName << ", offset=" << offset << ", byteCount=" << byteCount << std::endl;
    }

    std::string result;
    result.resize(byteCount);

    IFile ifile(HPIOpenFile(m_handle, fileName.c_str()));
    result.resize(byteCount);
    ifile.get((void*)result.data(), offset, byteCount);
    return result;
}

void IFile::get(void *dest, int offset, int bytecount)
{
    HPIGet(dest, m_handle, offset, bytecount);
}

bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(), b.begin(),
        [](char a, char b) {
        return tolower(a) == tolower(b);
    });
}

void HpiArchive::directory(
    std::map<std::string, HpiEntry> &entries,
    const std::string &gamePath, const std::string &hpiGlobSpec,
    const std::string &hpiSubDir, std::function<bool(const char*, bool)> match)
{
    if (VERBOSE)
    {
        std::cout << "[HpiArchive::directory] gamePath=" << gamePath << ", hpiGlobSpec=" << hpiGlobSpec << ", hpiSubDir=" << hpiSubDir << std::endl;
    }

    // TA does this in alphabetical order
    std::set<std::string> hpiFiles;
    {
        std::string spec = gamePath + "\\" + hpiGlobSpec;
        WIN32_FIND_DATA fd;
        HANDLE sh = FindFirstFile(spec.c_str(), &fd);
        if (sh == INVALID_HANDLE_VALUE)
        {
            return;
        }

        do
        {
            std::string hpiFile(fd.cFileName);
            std::transform(hpiFile.begin(), hpiFile.end(), hpiFile.begin(), [](char ch) { return ::tolower(ch); });
            hpiFiles.insert(fd.cFileName);
        }
        while (FindNextFile(sh, &fd));
    }

    int nrHpi = 0;
    for (const std::string& hpiFile : hpiFiles)
    {
        if (hpiFile.length()>4 && iequals(hpiFile.substr(hpiFile.length()-4, 4), ".hpi"))
        {
            ++nrHpi;
            if (nrHpi > 8)
            {
                // TA only loads the first 8 HPIs! go figure ....
                //continue;
            }
        }

        try
        {
            HpiArchive hpi(gamePath + "\\" + hpiFile);
            hpi.directory(entries, hpiSubDir, match);
        }
        catch (const std::exception & e)
        {
            if (VERBOSE)
            {
                std::cout << e.what() << std::endl;
            }
        }
        catch (...)
        {
            if (VERBOSE)
            {
                std::cout << "[HpiArchive::directory] general exception processing archive " << gamePath + "\\" + hpiFile << std::endl;
            }
        }

    }
}
