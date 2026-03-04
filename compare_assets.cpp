#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <memory>
#include <map>
#include <vector>
#include <sstream>

#include "rwe/hpi/HpiArchive.h"
#include "nswf/nswfl_crc32.h"

// Helper function to compute CRC32 using Qt's implementation
QString computeCrc32(const QByteArray& data)
{
    NSWFL::Hashing::CRC32 hasher;
    unsigned crc32 = hasher.FullCRC((const unsigned char*)data.data(), data.size());
    QByteArray ba((const char*)&crc32, 4);
    std::reverse(ba.begin(), ba.end());
    return QString(ba.toHex());
}

// Helper function to process archive files
void processArchive(const QString& archivePath, const QString& pattern, std::multimap<QString, QString>& targetMap)
{
    QFile archiveFile(archivePath);
    if (!archiveFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open archive:" << archivePath;
        return;
    }

    QByteArray archiveData = archiveFile.readAll();
    archiveFile.close();

    std::stringstream archiveStream(std::string(archiveData.constData(), archiveData.size()));
    std::unique_ptr<rwe::HpiArchive> archive;

    try
    {
        archive = std::make_unique<rwe::HpiArchive>(&archiveStream);
    }
    catch (const std::exception& e)
    {
        qWarning() << "Failed to parse archive" << archivePath << ":" << e.what();
        return;
    }

    // Recursive function to process directory entries
    std::function<void(const rwe::HpiArchive::Directory&, const QString&)> processDirectory;
    processDirectory = [&](const rwe::HpiArchive::Directory& dir, const QString& currentPath)
    {
        for (const auto& entry : dir.entries)
        {
            QString fullPath = currentPath + "/" + QString::fromStdString(entry.name);

            if (entry.file)
            {
                // Check if file matches pattern
                QFileInfo fileInfo(fullPath);
                if (!fileInfo.fileName().contains(QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption)))
                {
                    //qDebug() << "no match" << archivePath << fullPath;
                    continue;
                }

                // Read file contents
                std::vector<char> buffer(entry.file->size);
                archive->extract(*entry.file, buffer.data());

                // Calculate CRC32 using Qt's implementation
                QByteArray fileData(buffer.data(), buffer.size());
                QString crc = computeCrc32(fileData);

                // Add to map
                targetMap.insert({ crc, archivePath + ":" + fullPath });
                //qDebug() << "match" << archivePath << fullPath;
            }
            else if (entry.directory)
            {
                processDirectory(*entry.directory, fullPath);
            }
        }
    };

    processDirectory(archive->root(), "");
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("find-matches");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Find files with matching CRC32 hashes between source directory and target archives");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("source", "Source directory to scan for files");
    parser.addPositionalArgument("target", "Target directory containing archives to scan");
    parser.addPositionalArgument("mod", "show match results only where Target directory (or substring of) is exclusively this parameter. ie where the source file is ONLY found in this target subdirectory. can be a ; seperated list");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() != 3)
    {
        parser.showHelp(1);
    }

    QString sourceRoot = args[0];
    QString targetRoot = args[1];
    QStringList targetMods = args[2].split(';');

    QString globPattern = "*.*"; // "*." + pattern;
    QString rePattern = ".*"; // "\\." + pattern + "$";

    // Build CRC32 map for target files in archives
    std::multimap<QString, QString> targetMap;

    // Supported archive extensions
    QStringList archiveExtensions = { ".ccx", ".hpi", ".gp3", ".ufo" };

    QDir targetDir(targetRoot);

    // First process archives in the root directory
    foreach(const QString & ext, archiveExtensions)
    {
        QStringList archives = targetDir.entryList(QStringList() << "*" + ext, QDir::Files);
        foreach(const QString & archive, archives)
        {
            QString archivePath = targetDir.filePath(archive);
            qInfo() << "Processing archive:" << archivePath;
            processArchive(archivePath, rePattern, targetMap);
        }
    }

    // Then process archives in immediate subdirectories (one level deep)
    QStringList subDirs = targetDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach(const QString & subDir, subDirs)
    {
        QDir subTargetDir(targetDir.filePath(subDir));
        foreach(const QString & ext, archiveExtensions)
        {
            QStringList archives = subTargetDir.entryList(QStringList() << "*" + ext, QDir::Files);
            foreach(const QString & archive, archives)
            {
                QString archivePath = subTargetDir.filePath(archive);
                qInfo() << "Processing archive:" << archivePath;
                processArchive(archivePath, rePattern, targetMap);
            }
        }
    }

    // Check source files
    QDir sourceDir(sourceRoot);
    QFileInfoList sourceFiles;
    QDirIterator it(sourceDir.path(),
        QStringList() << globPattern,
        QDir::Files,
        QDirIterator::Subdirectories); // Critical flag for recursion

    while (it.hasNext()) {
        sourceFiles.append(QFileInfo(it.next()));
    }

    foreach(const QFileInfo & fileInfo, sourceFiles)
    {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Failed to open source file:" << fileInfo.absoluteFilePath();
            continue;
        }

        QByteArray fileData = file.readAll();
        QString crc = computeCrc32(fileData);

        bool isExclusive = true;
        for (int pass = 0; pass < 2; ++pass)
        {
            auto range = targetMap.equal_range(crc);
            if (range.first != range.second)
            {
                if (pass == 0)
                {
                    int countTargetModMatchFound = 0;
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        for (QString targetMod : targetMods)
                        {
                            if (targetMods.size() == 0 || it->second.contains(targetMod, Qt::CaseInsensitive))
                            {
                                ++countTargetModMatchFound;
                                break;
                            }
                        }
                    }
                    isExclusive = countTargetModMatchFound == std::distance(range.first, range.second);
                }
                else if (isExclusive)
                {
                    qInfo().noquote() << "\nMatch for:" << fileInfo.absoluteFilePath() << "(CRC32:" << crc << ")";
                    for (auto it = range.first; it != range.second; ++it)
                    {
                        qInfo().noquote() << "    =>" << it->second;
                    }
                }
            }
        }
    }

    return 0;
}