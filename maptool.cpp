#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <QtWidgets/qapplication.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qelapsedtimer.h>
#include <QtGui/qimage.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include "ta/tdf.h"
#include "ta/palette.h"
#include "nswf/nswfl_crc32.h"
#include "rwe/tnt/TntArchive.h"
#include "rwe/hpi/HpiArchive.h"
#include <set>

static bool VERBOSE = false;
#define LOG_DEBUG(x) if (VERBOSE) { std::cout << x << std::endl; }

static std::string toLower(const std::string & s)
{
    std::string lower;
    std::transform(s.begin(), s.end(), std::back_inserter(lower), ::tolower);
    return lower;
}

struct HpiArchiveRepository
{
    struct Archive
    {
        std::shared_ptr<std::ifstream> fs;
        std::shared_ptr<rwe::HpiArchive> hpi;
        std::string hpiPath;
    };

    static std::map<std::string, Archive> m_archives;

public:
    static rwe::HpiArchive& get(std::string path)
    {
        try
        {
            Archive& archive = m_archives[path];
            if (!archive.hpi)
            {
                archive.fs.reset(new std::ifstream(path, std::ios::binary));
                if (!(*archive.fs))
                {
                    std::ostringstream ss;
                    ss << "bad ifstream:" << archive.fs->rdstate() << ", errno:" << std::strerror(errno);
                    throw std::runtime_error(ss.str());
                }
                archive.hpi.reset(new rwe::HpiArchive(archive.fs.get()));
                archive.hpiPath = path;
            }
            return *archive.hpi;
        }
        catch (const std::exception& e)
        {
            std::ostringstream ss;
            ss << e.what() << "(" << path << ", m_archive.size()=" << m_archives.size() << ")";
            throw std::runtime_error(ss.str());
        }
    }
};

std::map<std::string, HpiArchiveRepository::Archive> HpiArchiveRepository::m_archives;

struct HpiEntry
{
    std::string archivePath;
    std::string filePath;   // original case
    std::size_t fileSize;
    std::shared_ptr<rwe::HpiArchive::File> file;
};

std::string hpiLoad(const HpiEntry &entry)
{
    LOG_DEBUG("[hpiLoad] " << entry.archivePath << ":" << entry.filePath);

    std::string data;
    {
        rwe::HpiArchive& hpi = HpiArchiveRepository::get(entry.archivePath);
        LOG_DEBUG("[hpiLoad] " << entry.archivePath << ":" << entry.filePath << ", size=" << entry.file->size);
        data.resize(entry.file->size);
        hpi.extract(*entry.file, const_cast<char*>(data.data()));
    }
    return data;
}

void HpiDirectory(
    std::map<std::string /* lower case */, HpiEntry>& entries,
    const rwe::HpiArchive::Directory& root,
    const std::string& archivePath,
    const std::string& filePathBase,
    std::function<bool(const char* /* fileName */, bool /* isDirectory */)> match)
{
    for (const rwe::HpiArchive::DirectoryEntry &dirEntry : root.entries)
    {
        bool isDirectory = dirEntry.directory.get() != NULL;
        std::string filePath = filePathBase + "/" + dirEntry.name;
        if (match(dirEntry.name.c_str(), isDirectory))
        {
            LOG_DEBUG("[HpiDirectory] FILE MATCH " << archivePath << ':' << filePath << (isDirectory ? "[DIRECTORY]" : "[FILE]"));
            HpiEntry& hpiEntry = entries[toLower(filePath)];
            hpiEntry.archivePath = archivePath;
            hpiEntry.filePath = filePath;
            hpiEntry.fileSize = dirEntry.file->size;
            hpiEntry.file = dirEntry.file;
        }
        else
        {
            LOG_DEBUG("[HpiDirectory] NO FILE MATCH " << archivePath << ':' << filePath << (isDirectory ? "[DIRECTORY]" : "[FILE]"));
        }

        if (isDirectory)
        {
            HpiDirectory(entries, *dirEntry.directory, archivePath, filePath, match);
        }
    }
}

void HpiDirectory(
    std::map<std::string /* lower case */, HpiEntry>& entries,
    const std::string& gamePath, const std::string& hpiGlobSpec, const std::string& hpiSubDir,
    std::function<bool(const char* /* fileName */, bool /* isDirectory */)> match)
{
    LOG_DEBUG("[HpiDirectory] gamePath=" << gamePath << ", hpiGlobSpec=" << hpiGlobSpec << ", hpiSubDir=" << hpiSubDir);

    // TA does this in alphabetical order
    QStringList hpiFiles = QDir(QString::fromStdString(gamePath)).entryList({ QString::fromStdString(hpiGlobSpec) }, QDir::Files, QDir::Name);

    int nrHpi = 0;
    for (QString hpiFile : hpiFiles)
    {
        if (QFileInfo(hpiFile).completeSuffix().toLower() == "hpi")
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
            std::string archivePath = gamePath + "/" + hpiFile.toStdString();
            rwe::HpiArchive& hpi = HpiArchiveRepository::get(archivePath);
            for (const rwe::HpiArchive::DirectoryEntry & hpiDirEntry: hpi.root().entries)
            {
                if (hpiDirEntry.directory && 0 == QString::fromStdString(hpiDirEntry.name).compare(QString::fromStdString(hpiSubDir), Qt::CaseInsensitive))
                {
                    LOG_DEBUG("[HpiDirectory] DIR MATCH " << hpiFile.toStdString() << ':' << hpiDirEntry.name << (hpiDirEntry.directory ? "[DIRECTORY]" : "[FILE]"));
                    HpiDirectory(entries, *hpiDirEntry.directory, archivePath, hpiDirEntry.name, match);
                }
                else
                {
                    LOG_DEBUG("[HpiDirectory] NO DIR MATCH " << hpiFile.toStdString() << ':' << hpiDirEntry.name << (hpiDirEntry.directory ? "[DIRECTORY]" : "[FILE]"));
                }
            }
        }
        catch (const std::exception & e)
        {
            LOG_DEBUG("[HpiDirectory] " << e.what());
        }
        catch (...)
        {
            LOG_DEBUG("[HpiDirectory] general exception processing archive " << gamePath << "/" << hpiFile.toStdString());
        }

    }
}

QVector<QRgb> loadPalette(const std::string &paletteFile)
{
    LOG_DEBUG("[loadPalette] paletteFile.size()=" << paletteFile.size());
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

QImage readMiniMap(const rwe::TntArchive& tnt)
{
    LOG_DEBUG("[readMiniMap] tnt:" << tnt.getHeader().width << 'x' << tnt.getHeader().height);
    rwe::TntMinimapInfo minimap = tnt.readMinimap();
    std::vector<char> *buf = new std::vector<char>(minimap.data);

    int miniHeight = minimap.height, miniWidth = minimap.width;
    LOG_DEBUG("  minimap:" << miniWidth << 'x' << miniHeight);
    {
        const char MAGIC_END(9);
        std::vector<bool> isRowAllMAGICEND(minimap.height, true);
        std::vector<bool> isColAllMAGICEND(minimap.width, true);
        for (std::size_t n = 0u; n < buf->size(); ++n)
        {
            std::size_t x = n % minimap.width;
            std::size_t y = n / minimap.width;
            if (buf->at(n) != MAGIC_END)
            {
                isColAllMAGICEND[x] = isRowAllMAGICEND[y] = false;

            }
        }

        auto itLastRowNotMAGICEND = std::find(isRowAllMAGICEND.rbegin(), isRowAllMAGICEND.rend(), false);
        if (itLastRowNotMAGICEND != isRowAllMAGICEND.rend() && itLastRowNotMAGICEND != isRowAllMAGICEND.rbegin())
        {
            std::size_t ofsRealMiniHeight = std::distance(isRowAllMAGICEND.begin(), itLastRowNotMAGICEND.base());
            miniHeight = ofsRealMiniHeight;
        }

        auto itLastColNotMAGICEND = std::find(isColAllMAGICEND.rbegin(), isColAllMAGICEND.rend(), false);
        if (itLastColNotMAGICEND != isColAllMAGICEND.rend() && itLastColNotMAGICEND != isColAllMAGICEND.rbegin())
        {
            std::size_t ofsRealMiniWidth = std::distance(isColAllMAGICEND.begin(), itLastColNotMAGICEND.base());
            miniWidth = ofsRealMiniWidth;
        }
    }
    LOG_DEBUG("  minimap(adjusted):" << miniWidth << 'x' << miniHeight);

    QImage im((std::uint8_t*)buf->data(), miniWidth, miniHeight, minimap.width, QImage::Format_Indexed8, &ImageBufferCleanup, buf);
    LOG_DEBUG("  image:" << im.width() << 'x' << im.height());
    return im;
}

std::vector<std::uint8_t> readHeightMap(const rwe::TntArchive& tnt)
{
    const int width = tnt.getHeader().width;
    const int height = tnt.getHeader().height;
    LOG_DEBUG("[readHeightMap] tnt:" << width << 'x' << height);
    std::vector<rwe::TntTileAttributes> tileAttributes(width * height);
    tnt.readMapAttributes(tileAttributes.data());

    std::vector<std::uint8_t> heights;
    heights.reserve(tileAttributes.size());
    std::transform(tileAttributes.begin(), tileAttributes.end(), std::back_inserter(heights), [](const rwe::TntTileAttributes& attr) {return attr.height; });
    return heights;
}

std::vector<std::uint8_t> lowpass(const std::vector<std::uint8_t> &data, int width, int height, int radius)
{
    LOG_DEBUG("[lowpass]");
    std::vector<std::uint8_t> result(data);
    for (int x = 0; x < width; ++x)
    {
        for (int y = 0; y < height; ++y)
        {
            double sum = 0.0;
            double sumWeights = 0.0;

            for (int xofs = -radius; xofs < radius; ++xofs)
            {
                for (int yofs = -radius; yofs < radius; ++yofs)
                {
                    int xn = x + xofs;
                    int yn = y + yofs;
                    if (xn >= 0 && yn >= 0 && xn < width && yn < height)
                    {
                        double weight = std::exp((-double(xofs * xofs) - double(yofs * yofs)) /double(radius) /2.0);
                        sum += weight * data[xn + yn * width];
                        sumWeights += weight;
                    }
                }
            }
            if (sumWeights > 0.0)
            {
                result[x + y * width] = int(0.5+ (sum / sumWeights));
            }
        }
    }
    return result;
}

void meanStdev(const std::vector<std::uint8_t>& data, double &mean, double &stdev)
{
    LOG_DEBUG("[meanStdev]");
    if (data.size() > 0)
    {
        double sum = std::accumulate(data.begin(), data.end(), 0.0, [](double a, std::uint8_t x) { return a + double(x); });
        double sumsq = std::accumulate(data.begin(), data.end(), 0.0, [](double a, std::uint8_t x) { return a + double(x) * double(x); });
        mean = sum / double(data.size());
        double meansq = sumsq / double(data.size());
        stdev = std::sqrt(meansq - mean * mean);
    }
    else
    {
        mean = 0.0;
        stdev = 0.0;
    }
}

QImage createHeightMapImage(const rwe::TntArchive& tnt)
{
    const int width = tnt.getHeader().width;
    const int height = tnt.getHeader().height;
    LOG_DEBUG("[createHeightMapImage] tnt:" << width << 'x' << height);
    std::vector<std::uint8_t> heights = readHeightMap(tnt);
    std::vector<std::uint8_t> lpfHeights = lowpass(heights, width, height, 3);
    
    double meanHeight, stdevHeight;
    meanStdev(lpfHeights, meanHeight, stdevHeight);

    QImage im(width, height, QImage::Format_RGB888);
    im.fill(65535u);
    for (int n=0; n<width*height; ++n)
    {
        int x = n % width;
        int y = n / width;
        std::uint8_t h = heights[n];
        if (stdevHeight > 1.0)
        {
            double z = (h - meanHeight) / stdevHeight;
            z = 128.0 + z * 128.0 / 3.0;
            h = std::min(std::max(z, 0.0), 255.0);
        }
        im.setPixel(x, y, qRgb(h, h, h));
    }
    return im;
}

template<typename IteratorT>
IteratorT findClosest(int x, int y, IteratorT beginNode, IteratorT endNode)
{
    return std::min_element(beginNode, endNode, [x, y](const std::pair<int, int>& node1, const std::pair<int, int>& node2) {
        double d1 = (node1.first - x) * (node1.first - x) + (node1.second - y) * (node1.second - y);
        double d2 = (node2.first - x) * (node2.first - x) + (node2.second - y) * (node2.second - y);
        return d1 < d2;
    });
}

template<typename IteratorT>
void voronoiLines(QImage& im, IteratorT beginNode, IteratorT endNode, QColor plotColor)
{
    LOG_DEBUG("[voronoiLines]");
    QPainter painter(&im);
    for (int x = 0; x < im.width(); ++x)
    {
        for (int y = 0; y < im.height(); ++y)
        {
            std::vector<double> distances;
            std::transform(beginNode, endNode, std::back_inserter(distances), [x, y](const std::pair<int, int>& node) { return std::sqrt((node.first - x) * (node.first - x) + (node.second - y) * (node.second - y)); });
            std::sort(distances.begin(), distances.end());
            if (std::abs(distances[0]-distances[1]) < 1.415)
            {
                painter.setPen(plotColor);
                painter.drawPoint(x, y);
            }
        }
    }
}

bool isSkirmishMap(const ta::TdfFile& ota)
{
    // "proper" method.  requires parsing of tdf deeper than level 1
    for (auto it = ota.children.begin(); it != ota.children.end(); ++it)
    {
        for (auto childIt = it->second.children.begin(); childIt != it->second.children.end(); ++childIt)
        {
            try
            {
                if (toLower(childIt->first.substr(0, 6)) == "schema" && childIt->second.getValue("type", "").substr(0, 7) == "network")
                {
                    return true;
                }
            }
            catch (const std::out_of_range &)
            {
            }
        }
    }

    return false;
}

bool isSkirmishMap(const std::string& otaData)
{
    // pragmatists method ...
    return QString(otaData.c_str()).contains("type=network", Qt::CaseInsensitive);
}

void lsMap(std::ostream& os, const std::string& context, const std::string &hpiArchive, const ta::TdfFile& ota, std::uint32_t crc)
{
    const char UNIT_SEPARATOR = '\x1f';
    const char RECORD_SEPARATOR = '\n';// '\x1e';
    for (auto it = ota.children.begin(); it != ota.children.end(); ++it)
    {
        try
        {
            auto tdfRootValues = it->second.values;
            QFileInfo hpiFileInfo(hpiArchive.c_str());

            std::ostringstream ss;
            ss << context
                << UNIT_SEPARATOR << hpiFileInfo.fileName().toStdString()
                << UNIT_SEPARATOR << std::hex << std::setw(8) << std::setfill('0') << crc
                << UNIT_SEPARATOR << tdfRootValues["missiondescription"]
                << UNIT_SEPARATOR << tdfRootValues["size"]
                << UNIT_SEPARATOR << tdfRootValues["numplayers"]
                << UNIT_SEPARATOR << tdfRootValues["minwindspeed"] + '-' + tdfRootValues["maxwindspeed"]
                << UNIT_SEPARATOR << tdfRootValues["tidalstrength"]
                << UNIT_SEPARATOR << tdfRootValues["gravity"]
                << RECORD_SEPARATOR;
            os << ss.str();
            return;
        }
        catch (std::exception &)
        { }
    }
    os << context << std::endl;
}

template<typename T>
T quote(const T& s)
{
    return '"' + s + '"';
}

template<typename T>
T sqlQuote(T s)
{
    return "'" + s.replace("'", "''") + "'";
}

template<typename T>
T bracket(const T& s)
{
    return '(' + s + ')';
}

int defaultInt(QString s, int def)
{
    bool ok;
    int result = s.toInt(&ok);
    return ok ? result : def;
}

std::ostream& operator<<(std::ostream& os, const QString& s)
{
    os << s.toStdString();
    return os;
}

void sqlMap(std::ostream& os, const std::string& context, const std::string &hpiArchive, const ta::TdfFile& ota, std::uint32_t crc)
{
    const int MAP_SIZE_SCALE_FACTOR = 1;
    try
    {
        std::ostringstream ss;
        for (auto it = ota.children.begin(); it != ota.children.end(); ++it)
        {
            QFileInfo hpiFileInfo(hpiArchive.c_str());
            QStringList values;
            values.append(sqlQuote(QString(context.c_str())));
            values.append(sqlQuote(QString("FFA")));
            values.append(sqlQuote(QString("skirmish")));

            ss << "INSERT INTO faf.map(display_name, map_type, battle_type) VALUES" << bracket(values.join(",")) << "\n";
            ss << "ON DUPLICATE KEY UPDATE display_name=" << values[0] << ", map_type=" << values[1] << ", battle_type=" << values[2] << ";\n";
        }

        for (auto it = ota.children.begin(); it != ota.children.end(); ++it)
        {
            auto tdfRootValues = it->second.values;
            QFileInfo hpiFileInfo(hpiArchive.c_str());
            QStringList values;
            values.append(sqlQuote(QString::fromStdString(tdfRootValues["missiondescription"])));
            values.append(QString::number(defaultInt(QString(tdfRootValues["numplayers"].c_str()).split(",").back().trimmed(), 10)));
            values.append(QString::number(MAP_SIZE_SCALE_FACTOR * defaultInt(QString(tdfRootValues["size"].c_str()).split("x").front().trimmed(), 16)));
            values.append(QString::number(MAP_SIZE_SCALE_FACTOR * defaultInt(QString(tdfRootValues["size"].c_str()).split("x").back().trimmed(), 16)));
            values.append("@version");
            values.append(sqlQuote(hpiFileInfo.fileName() + "/" + context.c_str() + "/" + QString("%1").arg(crc, 8, 16, QChar('0'))));
            values.append("1");
            values.append("0");
            values.append("(SELECT id FROM faf.map WHERE display_name=" + sqlQuote(QString(context.c_str())) + ")");

            ss << "INSERT INTO faf.map_version(description, max_players, width, height, version, filename, ranked, hidden, map_id) VALUES" << bracket(values.join(",")) << "\n";
            ss << "ON DUPLICATE KEY UPDATE description=" << values[0] << ", max_players=" << values[1] << ", width=" << values[2] << ", height=" << values[3] << ", version=" << values[4] << ", filename=" << values[5] << ", ranked=" << values[6] << ", hidden=" << values[7] << ", map_id=" << values[8] << ";\n";
        }

        os << ss.str();
    }
    catch (std::exception&)
    {
        os << "-- unable to generate sql for '" << context << "'\n";
    }

}

const ta::TdfFile* getSchema(const ta::TdfFile& root, const std::string &type)
{
    LOG_DEBUG("[getSchema] values=" << root.values.size() << ", children=" << root.children.size() << ", type=" << type);
    int schemaCount = std::atoi(root.getValue("schemacount", "0").c_str());

    for (int i = 0; i < schemaCount; ++i)
    {
        const ta::TdfFile& schema = root.getChild("schema " + std::to_string(i));
        if (schema.getValue("type", "").rfind(type) == 0) // aka startswith
        {
            return &schema;
        }
    }
    return NULL;
}

std::vector< std::pair<int, int> > getSchemaStartingPositions(const ta::TdfFile& schema)
{
    LOG_DEBUG("[getSchemaStartingPositions] values=" << schema.values.size() << ", children=" << schema.children.size());
    int MAX_START_POSITIONS = 10;
    std::vector< std::pair<int, int> > startPositions(MAX_START_POSITIONS, std::pair<int,int>(0,0));

    int schemaStartPositions = 0;
    for (int nSpecial = 0;; ++nSpecial)
    {
        const ta::TdfFile& special = schema.getChild("specials").getChild("special" + std::to_string(nSpecial));
        if (special.values.empty())
        {
            break;
        }

        if (special.getValue("specialwhat", "").rfind("startpos") == 0)
        {
            int positionNumber = std::atoi(special.getValue("specialwhat", "").substr(std::string("startpos").size()).c_str());
            int x = std::atoi(special.getValue("xpos", "-1").c_str());
            int y = std::atoi(special.getValue("zpos", "-1").c_str());
            if (positionNumber > 0 && positionNumber <= MAX_START_POSITIONS && x >= 0 && y >= 0)
            {
                startPositions[positionNumber - 1] = std::pair<int, int>(x / 16, y / 16);
                if (positionNumber > schemaStartPositions)
                {
                    schemaStartPositions = positionNumber;
                }
            }
        }
    }
    startPositions.resize(schemaStartPositions);
    return startPositions;
}

void appendSchemaFeatures(const ta::TdfFile& schema, std::vector< std::tuple<int, int, std::string> > &features)
{
    LOG_DEBUG("[appendSchemaFeatures] values=" << schema.values.size() << ", children=" << schema.children.size() << ", features=" << features.size());
    for (int nFeature = 0;; ++nFeature)
    {
        const ta::TdfFile& feature = schema.getChild("features").getChild("feature" + std::to_string(nFeature));
        if (feature.values.empty())
        {
            break;
        }

        std::string featureName = feature.getValue("featurename", "");
        int x = std::atoi(feature.getValue("xpos", "-1").c_str());
        int y = std::atoi(feature.getValue("zpos", "-1").c_str());
        if (featureName.size()>0 && x>=0 && y>=0)
        {
            features.push_back(std::tuple<int, int, std::string>(x, y, featureName));
        }
    }
}

const ta::TdfFile* getSchemaForPositionCount(const ta::TdfFile& ota, int positionCount, std::vector< std::pair<int,int> > *optionalStartingPositions)
{
    LOG_DEBUG("[getSchemaForPositionCount] values=" << ota.values.size() << ", children=" << ota.children.size() << ", positionCount=" << positionCount << ", optionalStartingPositons=" << optionalStartingPositions);
    std::size_t sizeLargestSchema = 0;
    const ta::TdfFile* largestSchema = NULL;

    for (auto& pairRoot : ota.children)
    {
        for (int nNetwork = 1;; ++nNetwork)
        {
            std::ostringstream ss;
            ss << "network " << nNetwork;
            const ta::TdfFile* schema = getSchema(pairRoot.second, ss.str());
            if (!schema)
            {
                break;
            }
            auto startPositions = getSchemaStartingPositions(*schema);
            if (startPositions.size() == positionCount)
            {
                if (optionalStartingPositions)
                {
                    *optionalStartingPositions = startPositions;
                }
                return schema;
            }
            if (startPositions.size() >= sizeLargestSchema)
            {
                sizeLargestSchema = startPositions.size();
                largestSchema = schema;
                if (optionalStartingPositions)
                {
                    *optionalStartingPositions = startPositions;
                }

            }
        }
    }
    return largestSchema;
}

std::vector< std::pair<int, int> > getStartingPositions(const ta::TdfFile &ota, int positionCount)
{
    LOG_DEBUG("[getStartingPositions] values=" << ota.values.size() << ", children=" << ota.children.size() << ", positionCount=" << positionCount);
    std::vector< std::pair<int, int> > startPositions;
    getSchemaForPositionCount(ota, positionCount, &startPositions);
    return startPositions;
}

void appendOtaFileFeatures(const ta::TdfFile& ota, int positionCount, std::vector< std::tuple<int, int, std::string> > &features)
{
    LOG_DEBUG("[appendOtaFileFeatures]");
    const ta::TdfFile *schema = getSchemaForPositionCount(ota, positionCount, NULL);
    if (schema)
    {
        appendSchemaFeatures(*schema, features);
    }
}

void appendTntFileFeatures(const rwe::TntArchive& tnt, std::vector< std::tuple<int, int, std::string> > &features)
{
    LOG_DEBUG("[appendTntFileFeatures]");
    int width = tnt.getHeader().width;
    int height = tnt.getHeader().height;
    std::vector<rwe::TntTileAttributes> tileAttributes(width * height);
    tnt.readMapAttributes(tileAttributes.data());

    std::vector<std::string> mapFeatures;
    tnt.readFeatures([&mapFeatures](const std::string& featureName) {
        mapFeatures.push_back(toLower(featureName));
    });
    //std::for_each(mapFeatures.begin(), mapFeatures.end(), [](std::string s) { std::cout << "mapfeature:" << s << std::endl; });

    for (unsigned n = 0u; n < tileAttributes.size(); ++n)
    {
        unsigned x = n % width;
        unsigned y = n / width;
        if (tileAttributes[n].feature < mapFeatures.size())
        {
            std::string featureName = mapFeatures[tileAttributes[n].feature];
            features.push_back(std::tuple<int, int, std::string>(x, y, featureName));
        }
    }
}

std::vector<std::tuple<int, int, int> > lookupFeatureValues(const std::vector< std::tuple<int, int, std::string> > &mapFeatures, const ta::TdfFile& featureLibrary, const std::string& matchKey, const std::string& matchValue, const std::string& valueKey)
{
    LOG_DEBUG("[lookupFeatureValues] matchKey=" << matchKey << ", matchValue=" << matchValue << ", valueKey=" << valueKey);
    std::vector<std::tuple<int, int, int> > matchingFeatureValues;
    for (const std::tuple<int, int, std::string> &featureTuple: mapFeatures)
    {
        int x = std::get<0>(featureTuple);
        int y = std::get<1>(featureTuple);
        const std::string& featureName = std::get<2>(featureTuple);
        const ta::TdfFile& featureData = featureLibrary.getChild(featureName);
        if (featureData.getValue(matchKey, "") == matchValue)
        {
            std::string valueString = featureData.getValue(valueKey, "");
            if (!valueString.empty())
            {
                int value = std::atoi(valueString.c_str());
                matchingFeatureValues.push_back(std::tuple<int, int, int>(x, y, value));
            }
        }
    }
    return matchingFeatureValues;
}

std::vector<int> voronoiAccumulateFeatures(const std::vector<std::tuple<int, int, int> > &featuresXYValue, const std::vector<std::pair<int, int> >& nodes)
{
    LOG_DEBUG("[voronoiAccumulateFeatures]");
    std::vector<int> areaValues(nodes.size(), 0);

    for (const std::pair<int, int> &xyVal : nodes)
    {
        LOG_DEBUG("  node(x,y)=" << std::get<0>(xyVal) << ',' << std::get<1>(xyVal));
    }

    for (const std::tuple<int,int,int> &xyVal: featuresXYValue)
    {
        int x = std::get<0>(xyVal);
        int y = std::get<1>(xyVal);
        int val = std::get<2>(xyVal);
        std::size_t closestNode = std::distance(nodes.begin(), findClosest(x, y, nodes.begin(), nodes.end()));
        LOG_DEBUG("  node=" << closestNode << " x=" << x << " y=" << y << " val=" << val);
        areaValues[closestNode] += val;
    }

    for (int sum : areaValues)
    {
        LOG_DEBUG("  sum(node)=" << sum);
    }
    return areaValues;
}

std::vector<double> weightedVoronoiAccumulateFeatures(const std::vector<std::tuple<int, int, int> >& featuresXYValue, const std::vector<std::pair<int, int> >& nodes)
{
    LOG_DEBUG("[weightedVoronoiAccumulateFeatures]");
    std::vector<double> sumValues(nodes.size(), 0);

    for (const std::tuple<int, int, int>& xyVal : featuresXYValue)
    {
        int x = std::get<0>(xyVal);
        int y = std::get<1>(xyVal);
        int val = std::get<2>(xyVal);

        std::multimap<double, std::size_t> nodesByDistance;
        for (std::size_t nNode=0u; nNode<nodes.size(); ++nNode)
        {
            int dx = x - std::get<0>(nodes[nNode]);
            int dy = y - std::get<1>(nodes[nNode]);
            double d = std::sqrt(dx * dx + dy * dy);
            nodesByDistance.insert(std::pair<double, std::size_t>(d, nNode));
        }
        std::size_t nClosestNode = nodesByDistance.begin()->second;
        double dClosestNode = nodesByDistance.begin()->first;
        double dSecondClosestNode = (std::next(nodesByDistance.begin()))->first;
        double weight = (dSecondClosestNode - dClosestNode)/dSecondClosestNode;
        weight = std::sin(3.141592654 * weight /2.0);
        sumValues[nClosestNode] += val * weight;
    }
    return sumValues;
}

void drawLabel(QPainter& painter, int x, int y, int size, QString text, Qt::GlobalColor back, Qt::GlobalColor fore)
{
    x = std::min(std::max(x, size), painter.device()->width()-size);
    y = std::min(std::max(y, size), painter.device()->height()-size);

    int ellipseWidth = 8*size/10;
    int ellipseHeight = 8*size / 10;
    int textWidth = 8 * size * text.size() / 10;
    int textHeight = size;

    QFont font;
    font.setPixelSize(size);
    QPainterPath path;
    path.addEllipse(x-ellipseWidth/2, 1+y-ellipseHeight/2, ellipseWidth, ellipseHeight);
    //path.addText(x - textWidth / 2, 1 + y + textHeight / 2, font, text);

    painter.setBrush(back);
    painter.setPen(fore);
    painter.drawPath(path);
    painter.setPen(fore);
    painter.drawText(1+x-textWidth/2, y-textHeight/2, textWidth, textHeight, Qt::AlignCenter, text);
}

void drawText(QPainter& painter, int x, int y, int size, QString text, Qt::GlobalColor back, Qt::GlobalColor fore)
{
    x = std::min(std::max(x, size), painter.device()->width()-size);
    y = std::min(std::max(y, size), painter.device()->height()-size);

    int textWidth = size * text.size()/2;
    int textHeight = size;

    QFont font;
    QPainterPath path;
    font.setPixelSize(size);
    path.addText(x- textWidth /2, y+ textHeight /2, font, text);

    painter.setPen(back);
    painter.setBrush(fore);
    painter.drawPath(path);
}

double resize(QImage& im, int nominalSize)
{
    LOG_DEBUG("[resize(QImage)]");
    int currentSize = std::max(im.width(), im.height());
    double scale = double(nominalSize) / double(currentSize);

    im = im.scaled(int(scale * double(im.width()) + 0.5), int(scale * double(im.height()) + 0.5));
    return scale;
}

QImage createPositionsMapImage(const rwe::TntArchive& tnt, const ta::TdfFile& ota, QVector<uint> palette, int positionCount, int nominalSize)
{
    LOG_DEBUG("[createPositionsMapImage]");
    QImage im;
    im = readMiniMap(tnt);
    double scale = double(im.width()) / double(tnt.getHeader().width);
    im.setColorTable(palette);
    im = im.convertToFormat(QImage::Format_RGB888);
    scale *= resize(im, nominalSize);

    QPainter painter(&im);

    const std::vector< std::pair<int, int> > startPositions = getStartingPositions(ota, positionCount);

    int positionNumber = startPositions.size();
    for (auto it=startPositions.rbegin(); it!=startPositions.rend(); ++it, --positionNumber)
    {
        Qt::GlobalColor back = positionNumber <= positionCount ? Qt::darkBlue : Qt::gray;
        Qt::GlobalColor fore = positionNumber <= positionCount ? Qt::white : Qt::black;
        drawLabel(painter, int(scale* it->first+0.5), int(scale* it->second+0.5), 20, QString::number(positionNumber), back, fore);
    }

    return im;
}

QString _engineeringNotation(double x, char k)
{
    if (x >= 1e15)
    {
        return QString("%1").arg(x, 0, 'g', 2);
    }
    if (x >= 1e12)
    {
        return _engineeringNotation(x / 1e12, 'T');
    }
    if (x >= 1e9)
    {
        return _engineeringNotation(x / 1e9, 'G');
    }
    else if (x >= 1e6)
    {
        return _engineeringNotation(x / 1e6, 'M');
    }
    else if (x >= 1e3)
    {
        return _engineeringNotation(x / 1e3, 'k');
    }
    else if (x >= 10.0 && k == 0)
    {
        return QString::number(int(x));
    }
    else if (x >= 10.0)
    {
        return _engineeringNotation(x, 0) + k;
    }
    else if (x >= 1.0 && k == 0)
    {
        QString s = QString("%1").arg(x).mid(0,3);
        while (s.contains('.') && (*s.rbegin() == '0' || *s.rbegin() == '.'))
        {
            s = s.mid(0, s.size() - 1);
        }
        return s;
    }
    else if (x >= 1.0)
    {
        QString s = _engineeringNotation(x, 0);
        return s.contains('.')
            ? s.replace('.', k)
            : s + k;
    }
    else if (x > 0.0)
    {
        QString s = QString("%1").arg(x).mid(0, 4);
        while (s.contains('.') && (*s.rbegin() == '0' || *s.rbegin() == '.'))
        {
            s = s.mid(0, s.size() - 1);
        }
        return s;
    }
    else if (x == 0.0)
    {
        return "0";
    }
    else
    {
        return "-" + _engineeringNotation(-x, k);
    }
}

double twoOrThreSigFigs(double x)
{
    if (x == 0.0)
    {
        return 0.0;
    }

    int order = std::floor(std::log(x) / std::log(10.0));
    if (order % 3 == 2)
    {
        order -= 2;
    }
    else // including -ve
    {
        order -= 1;
    }
    x += std::pow(10.0, order) / 2.0;
    return x;
}

QString engineeringNotation(double x)
{
    QString s = _engineeringNotation(twoOrThreSigFigs(x), 0).replace('.', ',');
    while (s[0] == '0' && s.size()>1)
    {
        s = s.mid(1, s.length() - 1);
    }
    return s;
}

std::vector<std::tuple<int, int, int> > normaliseFeatures(const std::vector<std::tuple<int, int, int> >& features)
{
    LOG_DEBUG("[normaliseFeatures]");
    if (features.empty())
    {
        return features;
    }

    int minVal = std::get<2>(*std::min_element(features.begin(), features.end(), [](const std::tuple<int, int, int>& a, const std::tuple<int, int, int>& b)
    {
        return std::get<2>(a) < std::get<2>(b);
    }));
    minVal = std::max(minVal, 1);

    std::vector<std::tuple<int, int, int> > normalisedFeatures;
    std::transform(features.begin(), features.end(), std::back_inserter(normalisedFeatures), [minVal](const std::tuple<int, int, int>& f)
    {
        int x = std::get<0>(f);
        int y = std::get<1>(f);
        int v = std::max(std::get<2>(f),1);
        return std::tuple<int, int, int>(x, y, int(std::log(double(v) / double(minVal))));
    });

    return normalisedFeatures;
}

QImage createResourceMapImage(const rwe::TntArchive& tnt, const ta::TdfFile& ota, const ta::TdfFile& featureLibrary, int maxPositions, Qt::GlobalColor background, Qt::GlobalColor foreground,
    const std::string &matchKey, const std::string &matchValue, const std::string &valueKey, int resourceScaleFactor, int nominalSize)
{
    LOG_DEBUG("[createResourceMapImage] matchKey=" << matchKey << ", matchValue=" << matchValue << ", valueKey=" << valueKey);

    Qt::GlobalColor summaryColour = foreground; // Qt::GlobalColor(int(Qt::transparent) - int(foreground));
    QImage im = createHeightMapImage(tnt);
    double scale = resize(im, nominalSize);

    std::vector<std::pair<int, int> > startPositions = getStartingPositions(ota, maxPositions);
    if (startPositions.size() > unsigned(maxPositions))
    {
        startPositions.resize(maxPositions);
    }
    {
        std::vector<std::pair<int, int> > scaledStartPositions;
        std::transform(startPositions.begin(), startPositions.end(), std::back_inserter(scaledStartPositions), [scale](const std::pair<int, int>& x) {return std::pair<int, int>(int(0.5 + x.first * scale), int(0.5 + x.second * scale)); });
        voronoiLines(im, scaledStartPositions.begin(), scaledStartPositions.end(), summaryColour);
    }

    std::vector< std::tuple<int, int, std::string> > mapFeatures;
    appendTntFileFeatures(tnt, mapFeatures);
    appendOtaFileFeatures(ota, maxPositions, mapFeatures);
    auto matchingFeatures = lookupFeatureValues(mapFeatures, featureLibrary, matchKey, matchValue, valueKey);
    auto normalisedMatchingFeatures = normaliseFeatures(matchingFeatures);
    auto areaValues = voronoiAccumulateFeatures(matchingFeatures, startPositions);
    //auto areaValues = weightedVoronoiAccumulateFeatures(matchingFeatures, startPositions);

    QPainter painter(&im);
    for (const auto& feature : normalisedMatchingFeatures)
    {
        int x = scale*std::get<0>(feature);
        int y = scale*std::get<1>(feature);
        int val = std::get<2>(feature);
        painter.setBrush(foreground);
        painter.setPen(background);
        painter.drawEllipse(x, y, 3+val, 3+val);
    }

    for (std::size_t n = 0; n < startPositions.size(); ++n)
    {
        int x = startPositions[n].first;
        int y = startPositions[n].second;
        double v = double(areaValues[n]) / double(resourceScaleFactor);
        QString value = engineeringNotation(v);
        LOG_DEBUG("drawText pos:" << n << " x:" << x << " y:" << y << " v:" << v << " value:" << value);
        drawText(painter, scale * x, scale * y, 30, value, Qt::black, summaryColour);
    }

    return im;
}

QImage createMapImage(const rwe::TntArchive& tnt, const ta::TdfFile& ota, const ta::TdfFile& allFeatures, QVector<uint> palette, QString type, int maxPositions, int nominalSize)
{
    LOG_DEBUG("[createMapImage] type=" << type.toStdString() << ", maxPositions=" << maxPositions << ", nominalSize=" << nominalSize);
    QImage im;
    if (type == "mini")
    {
        QImage im = readMiniMap(tnt);
        im.setColorTable(palette);
        return im;
    }
    else if (type == "positions")
    {
        return createPositionsMapImage(tnt, ota, palette, maxPositions, nominalSize);
    }
    else if (type == "mexes")
    {
        return createResourceMapImage(tnt, ota, allFeatures, maxPositions, Qt::darkRed, Qt::yellow, "indestructible", "1", "metal", 111, nominalSize);
    }
    else if (type == "geos")
    {
        return createResourceMapImage(tnt, ota, allFeatures, maxPositions, Qt::darkBlue, Qt::cyan, "indestructible", "1", "geothermal", 1, nominalSize);
    }
    else if (type == "rocks")
    {
        return createResourceMapImage(tnt, ota, allFeatures, maxPositions, Qt::darkRed, Qt::red, "reclaimable", "1", "metal", 1, nominalSize);
    }
    else if (type == "trees")
    {
        return createResourceMapImage(tnt, ota, allFeatures, maxPositions, Qt::darkGreen, Qt::green, "reclaimable", "1", "energy", 1, nominalSize);
    }
    else
    {
        return QImage();
    }
}


bool isInterestingFeature(const ta::TdfFile& tdf)
{
    return
        tdf.getValue("category", "") == "metal" ||
        tdf.getValue("category", "") == "steamvents" ||
        tdf.getValue("metal", "").size()>0 ||
        tdf.getValue("geothermal", "").size()>0 ||
        tdf.getValue("reclaimable", "") == "1";
}

std::map<QString,QImage> createMapImages(const std::string& tntData, const std::string& otaData, const ta::TdfFile& allFeatures, QVector<uint> palette, QStringList types, int maxPositions, int nominalSize)
{
    LOG_DEBUG("[createMapImages]");
    std::istringstream ss(tntData);
    rwe::TntArchive tnt(&ss);
    ta::TdfFile ota(otaData, 10);

    std::map<QString, QImage> images;

    for (QString type : types)
    {
        if (type == "mini")
        {
            images[type] = createMapImage(tnt, ota, allFeatures, palette, type, 10, nominalSize);
        }
        else
        {
            QString key = type + '_' + QString::number(maxPositions);
            images[key] = createMapImage(tnt, ota, allFeatures, palette, type, maxPositions, nominalSize);
        }
    }
    return images;
}

void saveMapImages(QFileInfo mapFileInfo, QString directory, const std::map<QString, QImage>& images)
{
    LOG_DEBUG("[saveMapImages] mapFileInfo=" << mapFileInfo.fileName().toStdString() << ", directory=" << directory.toStdString());
    QString fileName = mapFileInfo.baseName();

    for (const auto& p : images)
    {
        QString previewType = p.first;
        QImage image = p.second;

        QDir dir(directory + "/" + previewType);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        QString pngFileName = directory + "/" + previewType + "/" + fileName + ".png";
        LOG_DEBUG("[saveMapImages] pngFileName=" << pngFileName.toStdString());
        image.save(pngFileName);
    }
}

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--verbose")
        {
            VERBOSE = true;
            break;
        }
    }

    LOG_DEBUG("-- app start. default maxstdio=" + std::to_string(_getmaxstdio()));

    int maxstdio = 65536;
    while (_setmaxstdio(maxstdio) != maxstdio)
    {
        maxstdio = 9 * maxstdio / 10;
    }
    {
        LOG_DEBUG("set maxstdio=" + std::to_string(_getmaxstdio()));
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName("MapTool");

    QCommandLineParser parser;
    parser.setApplicationDescription("Maptool for working with TA maps");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("gamepath", "Path in which TA is located.", "gamepath"));
    parser.addOption(QCommandLineOption("hpispecs", "Search specs for HPI files.", "hpispecs", "*.hpi;*.gpf;rev31.gp3;*.ccx;*.ufo"));
    parser.addOption(QCommandLineOption("mapname", "map names to match (starts with).", "mapname", ""));
    parser.addOption(QCommandLineOption("hash", "Calculate hash for the map(s)."));
    parser.addOption(QCommandLineOption("thumb", "Create thumbnail image(s) for the map(s) in the given directory.", "thumb", "./"));
    parser.addOption(QCommandLineOption("thumbtypes", "comma separated list of preview types.", "thumbtypes", "mini,positions,mexes,geos,rocks,trees"));
    parser.addOption(QCommandLineOption("maxpositions", "maximum number of player positions to analyse for.", "maxpositions", "10"));
    parser.addOption(QCommandLineOption("thumbsize", "nominal size of thumbnail image.", "thumbsize", "375"));
    parser.addOption(QCommandLineOption("sql", "output map info in SQL format suitable for insertion into TAF DB.  argument specifies map version to use."));
    parser.addOption(QCommandLineOption("featurescachedir", "load TA features and cache them for future use when generating thumbnails", "featurescachedir"));
    parser.addOption(QCommandLineOption("verbose", "spit out some debugging information"));
    parser.process(app);

    NSWFL::Hashing::CRC32 crc32;
    crc32.Initialize();

    // map from asset filename to archive filename
    std::map<std::string, HpiEntry> mapFiles;
    std::map<std::string, HpiEntry> paletteFiles;
    std::map<std::string, HpiEntry> featureFiles;
    QString mapName = parser.value("mapname");

    const bool doHash = parser.isSet("hash") || parser.isSet("sql");
    const bool doLoadFeatures =
        parser.isSet("featurescachedir") ||
        parser.isSet("thumb") && (
            parser.value("thumbtypes").contains("mexes") ||
            parser.value("thumbtypes").contains("geos") ||
            parser.value("thumbtypes").contains("rocks") ||
            parser.value("thumbtypes").contains("trees"));

    LOG_DEBUG("--- inspecting hpi archives ...");
    for (QString hpiSpec : parser.value("hpispecs").split(';'))
    {
        HpiDirectory(mapFiles, parser.value("gamepath").toStdString(), hpiSpec.toStdString(), "maps",
            [&mapName](const char *fileName, bool isDirectory)
        {
            QFileInfo fileInfo(fileName);
            bool ok = !isDirectory && (
                mapName.isEmpty() ||
                fileInfo.baseName().startsWith(mapName, Qt::CaseInsensitive) ||
                mapName[mapName.size()-1]=='$' && mapName.mid(0,mapName.size()-1)==fileInfo.baseName());
            return ok;
        });

        HpiDirectory(paletteFiles, parser.value("gamepath").toStdString(), hpiSpec.toStdString(), "palettes",
            [](const char *fileName, bool isDirectory) { return !isDirectory; });

        if (doLoadFeatures)
        {
            HpiDirectory(featureFiles, parser.value("gamepath").toStdString(), hpiSpec.toStdString(), "features",
                [](const char* fileName, bool isDirectory) {
                return !isDirectory;
            });
        }
    }

    ta::TdfFile allFeatures;
    if (doLoadFeatures)
    {
        LOG_DEBUG("--- calculating feature files CRC ...");
        std::uint32_t crcFeatureFiles(-1);
        for (const auto& p : featureFiles)
        {
            std::string filePath = p.second.filePath;
            crc32.PartialCRC(&crcFeatureFiles, (const unsigned char*)filePath.data(), filePath.size());
            crc32.PartialCRC(&crcFeatureFiles, (const unsigned char*)&p.second.fileSize, sizeof(p.second.fileSize));
        }

        QString allFeaturesCacheFile(parser.value("featurescachedir") + "/" + "tafeatures." + QString::number(crcFeatureFiles, 16));
        if (QFile(allFeaturesCacheFile).exists())
        {
            LOG_DEBUG("--- loading cached features. filename=" << allFeaturesCacheFile.toStdString());
            std::ifstream ifs(allFeaturesCacheFile.toStdString(), std::ios::binary);
            allFeatures.deserialise(ifs);
        }
        else
        {
            LOG_DEBUG("--- loading features from hpi archives ...");
            for (const auto& p : featureFiles)
            {
                try
                {
                    QFileInfo fileInfo(p.second.filePath.c_str());
                    std::string tdfData = hpiLoad(p.second);

                    LOG_DEBUG("  parsing features");
                    ta::TdfFile features(tdfData, 1);

                    LOG_DEBUG("  filtering for interesting features");
                    for (const auto& f : features.children)
                    {
                        if (isInterestingFeature(f.second))
                        {
                            allFeatures.children[f.first] = f.second;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_DEBUG("  exception loading/parsing file:" << e.what());
                    continue;
                }
                catch (...)
                {
                    LOG_DEBUG("  exception loading/parsing file");
                    continue;
                }
            }
            if (parser.isSet("featurescachedir"))
            {
                LOG_DEBUG("--- saving features to cache. filename=" << allFeaturesCacheFile.toStdString() << ", values:" << allFeatures.values.size() << ", children:" << allFeatures.children.size());
                std::ofstream ofs(allFeaturesCacheFile.toStdString(), std::ios::binary);
                allFeatures.serialise(ofs);
            }
        }
    }

    std::map<std::string, std::uint32_t> crcByMap;
    if (parser.isSet("thumb") || doHash)
    {
        std::map<QString, std::string> otaFileNameLookup;
        LOG_DEBUG("--- populating otaFileNameLookup");
        for (const auto& p : mapFiles)
        {
            QFileInfo fileInfo(p.second.filePath.c_str());
            if (fileInfo.suffix().toLower() == "ota")
            {
                otaFileNameLookup[fileInfo.baseName()] = p.first;
            }
        }

        QVector<QRgb> palette;
        if (parser.isSet("thumb")) {
            std::string paletteData((const char*)ta::PALETTE, sizeof(ta::PALETTE));
            try
            {
                LOG_DEBUG("  loading palette");
                const HpiEntry &hpiEntry = paletteFiles.at("palettes/PALETTE.PAL");
                paletteData = hpiLoad(hpiEntry);
            }
            catch (std::out_of_range&)
            {
                LOG_DEBUG("  out_of_range loading palette. using hard coded palette");
            }
            catch (...)
            {
                LOG_DEBUG("  unknown exception loading palette. using hard coded palette");
            }
            palette = loadPalette(paletteData);
        }

        for (const auto &p : mapFiles)
        {
            try
            {
                QFileInfo fileInfo(p.second.filePath.c_str());
                if (fileInfo.suffix().toLower() == "tnt")
                {
                    LOG_DEBUG("--- processing map file " << p.second.filePath);
                    std::string otaFileName = otaFileNameLookup[fileInfo.baseName()];
                    std::string tntData, otaData;

                    const HpiEntry &mapFileHpiEntry = mapFiles.at(otaFileName);
                    tntData = hpiLoad(p.second);
                    otaData = hpiLoad(mapFileHpiEntry);

                    if (doHash)
                    {
                        LOG_DEBUG("  calculating .tnt part of CRC");
                        std::uint32_t crc(-1);
                        crc32.PartialCRC(&crc, (const std::uint8_t*)tntData.data(), tntData.size());
                        crcByMap[fileInfo.baseName().toStdString()] = crc;
                    }
                    if (parser.isSet("thumb"))
                    {
                        LOG_DEBUG("  generating map images");
                        auto images = createMapImages(tntData, otaData, allFeatures, palette, parser.value("thumbtypes").split(','), parser.value("maxpositions").toInt(), parser.value("thumbsize").toInt());
                        LOG_DEBUG("  saving map images");
                        saveMapImages(fileInfo, parser.value("thumb"), images);
                    }
                }
            }
            catch (const std::exception & e)
            {
                LOG_DEBUG("  exception processing map file " << p.second.archivePath << '/' << p.second.filePath << ":" << e.what());
                continue;
            }
            catch (...)
            {
                LOG_DEBUG("  unknown exception processing map file " << p.second.archivePath << '/' << p.second.filePath);
                continue;
            }
        }
    }

    for (const auto &p : mapFiles)
    {
        try
        {
            QFileInfo fileInfo(p.second.filePath.c_str());
            if (fileInfo.suffix().toLower() == "ota")
            {

                std::string data = hpiLoad(p.second);
                bool isSkirmish = isSkirmishMap(data);
                if (!isSkirmish)
                {
                    LOG_DEBUG("  not a skirmish map. ignoring");
                    continue;
                }

                std::uint32_t& crc = crcByMap[fileInfo.baseName().toStdString()];
                if (doHash)
                {
                    LOG_DEBUG("  calculating .ota part of CRC");
                    crc32.PartialCRC(&crc, (const std::uint8_t*)data.data(), data.size());
                    crc ^= -1;
                }

                LOG_DEBUG("  parsing .ota file");
                ta::TdfFile tdf(data, 1);
                if (parser.isSet("sql"))
                {
                    LOG_DEBUG("  listing file (sql)");
                    sqlMap(std::cout, fileInfo.baseName().toStdString(), p.second.archivePath, tdf, crc);
                }
                else
                {
                    LOG_DEBUG("  listing file (delimited text)");
                    lsMap(std::cout, fileInfo.baseName().toStdString(), p.second.archivePath, tdf, doHash ? crc : 0u);
                }
            }
        }
        catch (const std::exception & e)
        {
            LOG_DEBUG("  exception generating hash for map file " << p.second.archivePath << '/' << p.second.filePath << ":" << e.what());
            continue;
        }
        catch (...)
        {
            LOG_DEBUG("  unknown exception generating hash for map file " << p.second.archivePath << '/' << p.second.filePath);
            continue;
        }
    }
}
