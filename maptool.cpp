#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qfileinfo.h>
#include <QtGui/qimage.h>
#include <Windows.h>
#include "ta/hpi.h"
#include "ta/tdf.h"
#include "nswf/nswfl_crc32.h"
#include "rwe/tnt/TntArchive.h"
#include "rwe/hpi/HpiArchive.h"

std::string load(const ta::HpiEntry &hpiEntry)
{
    static std::shared_ptr<std::istream> fs;
    static std::shared_ptr<rwe::HpiArchive> hpi;
    static std::string hpiPath;

    if (hpiPath != hpiEntry.hpiArchive)
    {
        fs.reset(new std::ifstream(hpiEntry.hpiArchive, std::ios::binary));
        hpi.reset(new rwe::HpiArchive(fs.get()));
        hpiPath = hpiEntry.hpiArchive;
    }

    std::string filename = hpiEntry.fileName;
    std::replace(filename.begin(), filename.end(), '\\', '/');
    const rwe::HpiArchive::File *file = hpi->findFile(filename);

    std::string data;
    data.resize(hpiEntry.size);
    hpi->extract(*file, const_cast<char*>(data.data()));
    return data;
}

QVector<QRgb> loadPalette(const std::string &paletteFile)
{
    const QRgb *abgr = (const QRgb*)paletteFile.data();
    int N = paletteFile.size() / 4u;

    QVector<QRgb> palette;
    palette.reserve(N);

    for (int n=0; n<N; ++n)
    {
        QRgb rgb = 0xff000000;
        rgb |= (abgr[n] >> 16) & 0x000000ff;
        rgb |=  abgr[n]        & 0x0000ff00;
        rgb |= (abgr[n] << 16) & 0x00ff0000;
        palette.push_back(rgb);
    }

    return palette;
}

void ImageBufferCleanup(void *buf)
{
    delete buf;
}

QImage readMiniMap(const std::string &tntFile)
{
    std::istringstream ss(tntFile);
    rwe::TntArchive tnt(&ss);
    rwe::TntMinimapInfo minimap = tnt.readMinimap();

    std::vector<char> *buf = new std::vector<char>(minimap.data);
    return QImage((std::uint8_t*)buf->data(), minimap.width, minimap.height, minimap.width, QImage::Format_Indexed8, &ImageBufferCleanup, buf);
}

std::string quote(const std::string &s)
{
    return '"' + s + '"';
}

void lsMap(std::ostream &os, const std::string &context, ta::TdfFile &ota, std::uint32_t crc)
{
    for (auto it = ota.children.begin(); it != ota.children.end(); ++it)
    {
        try
        {
            const ta::TdfFile &data = it->second;
            std::ostringstream ss;
            ss << quote(context)
                << ',' << std::hex << std::setw(8) << std::setfill('0') << crc
                << ',' << quote(data.values.at("missiondescription"))
                << ',' << quote(data.values.at("size"))
                << ',' << quote(data.values.at("numplayers"))
                << ',' << quote(data.values.at("minwindspeed") + '-' + data.values.at("maxwindspeed"))
                << ',' << quote(data.values.at("tidalstrength"))
                << ',' << quote(data.values.at("gravity"))
                << std::endl;
            os << ss.str();
            return;
        }
        catch (std::exception &)
        { }

    }
    os << quote(context) << std::endl;;
}


int main(int argc, char *argv[])
{
    NSWFL::Hashing::CRC32 crc32;
    crc32.Initialize();
    ta::init();

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("MapTool");

    QCommandLineParser parser;
    parser.setApplicationDescription("Maptool for working with TA maps");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("gamepath", "Path in which TA is located", "gamepath"));
    parser.addOption(QCommandLineOption("hpispecs", "Search specs for HPI files", "hpispec", "*.ufo;*.hpi;*.ccx;*.gpf;rev31.gp3"));
    parser.addOption(QCommandLineOption("mapname", "map names to match (starts with)", "mapname", ""));
    parser.addOption(QCommandLineOption("hash", "Calculate hash for the map(s)"));
    parser.addOption(QCommandLineOption("details", "Parse the OTA file for more map details"));
    parser.addOption(QCommandLineOption("thumb", "Create thumbnail image(s) for the map(s) in the given directory", "thumb", ".\\"));
    parser.process(app);

    std::map<std::string, ta::HpiEntry> mapFiles;
    std::map<std::string, ta::HpiEntry> paletteFiles;
    QString mapName = parser.value("mapname");

    for (QString hpiSpec : parser.value("hpispecs").split(';'))
    {
        ta::HpiArchive::directory(mapFiles, parser.value("gamepath").toStdString(), hpiSpec.toStdString(), "maps",
            [&mapName](const char *fileName, bool isDirectory)
        {
            QFileInfo fileInfo(fileName);
            return !isDirectory && (mapName.isEmpty() || fileInfo.baseName().startsWith(mapName, Qt::CaseInsensitive));
        });

        ta::HpiArchive::directory(paletteFiles, parser.value("gamepath").toStdString(), hpiSpec.toStdString(), "palettes",
            [](const char *fileName, bool isDirectory) { return true; });
    }

    std::map<std::string, std::uint32_t> crcByMap;
    if (parser.isSet("thumb") || parser.isSet("hash"))
    {
        QVector<QRgb> palette = loadPalette(load(paletteFiles.at("palettes\\PALETTE.PAL")));
        for (auto p : mapFiles)
        {
            QFileInfo fileInfo(p.second.fileName.c_str());
            if (fileInfo.suffix().toLower() == "tnt")
            {
                std::string data;
                try
                {
                    data = load(p.second);
                }
                catch (const std::exception &)
                {
                    //std::cerr << e.what() << std::endl;
                    continue;
                }

                if (parser.isSet("hash"))
                {
                    std::uint32_t crc(-1);
                    crc32.PartialCRC(&crc, (const std::uint8_t*)data.data(), data.size());
                    crcByMap[fileInfo.baseName().toStdString()] = crc;
                }
                if (parser.isSet("thumb"))
                {
                    QImage im = readMiniMap(data);
                    im.setColorTable(palette);

                    QString fileNameEncoded = fileInfo.baseName();
                    fileNameEncoded.replace(" ", "%20");
                    fileNameEncoded.replace("[", "%5b");
                    fileNameEncoded.replace("]", "%5d");
                    QString pngFileName = parser.value("thumb") + "\\" + fileNameEncoded + ".png";
                    im.save(pngFileName);
                }
            }
        }
    }

    for (auto p : mapFiles)
    {
        QFileInfo fileInfo(p.second.fileName.c_str());
        if (fileInfo.suffix().toLower() == "ota")
        {
            std::uint32_t &crc = crcByMap[fileInfo.baseName().toStdString()];
            std::string data = load(p.second);
            if (parser.isSet("hash") || parser.isSet("details"))
            {
                crc32.PartialCRC(&crc, (const std::uint8_t*)data.data(), data.size());
                crc ^= -1;
            }
            ta::TdfFile tdf(data, parser.isSet("details") ? 1 : 0);
            lsMap(std::cout, fileInfo.baseName().toStdString(), tdf, parser.isSet("hash") ? crc : 0u);
        }
    }
}
