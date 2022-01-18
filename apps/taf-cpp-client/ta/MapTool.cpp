#include "MapTool.h"

#include "taflib/Logger.h"

#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qprocess.h>

MapTool* MapTool::m_instance = NULL;

MapTool* MapTool::initialise(QString mapToolExePath, QString cacheDirectory)
{
    return m_instance = new MapTool(mapToolExePath, cacheDirectory);
}

MapTool* MapTool::getInstance()
{
    return m_instance;
}

MapTool::MapTool(QString mapToolExePath, QString cacheDirectory):
    m_mapToolExePath(mapToolExePath),
    m_cacheDirectory(cacheDirectory)
{
    if (!QFile(mapToolExePath).exists())
    {
        throw std::runtime_error(QString("No MapTool not found at path '%1'").arg(mapToolExePath).toStdString());
    }
    if (!QDir().mkpath(cacheDirectory))
    {
        throw std::runtime_error(QString("Unable to create cache path '%1'").arg(cacheDirectory).toStdString());
    }
}

MapListSignal* MapTool::run(
    QString mapToolExePath, QString gamePath, QString hpiSpecs, QString mapName, bool doCrc,
    QString previewCacheDirectory, QString previewType, int maxPositions, QString featuresCacheDirectory)
{
    QStringList arguments;
    arguments << "--gamepath" << gamePath;
    if (!hpiSpecs.isEmpty())
    {
        arguments << "--hpispecs" << hpiSpecs;
    }
    if (!mapName.isEmpty())
    {
        arguments << "--mapname" << mapName;
    }
    if (doCrc)
    {
        arguments << "--hash";
    }
    if (!previewCacheDirectory.isEmpty())
    {
        arguments << "--thumb" << previewCacheDirectory;
    }
    if (!previewType.isEmpty())
    {
        arguments << "--thumbtypes" << previewType;
    }
    if (maxPositions > 0)
    {
        arguments << "--maxpositions" << QString::number(maxPositions);
    }
    if (!featuresCacheDirectory.isEmpty())
    {
        arguments << "--featurescachedir" << featuresCacheDirectory;
    }

    MapListSignal* result(new MapListSignal);
    QProcess* process(new QProcess(result));
    process->setWorkingDirectory(QFileInfo(mapToolExePath).dir().absolutePath());

    QSharedPointer<QList<MapToolDto> > mapList(new QList<MapToolDto>);

    QObject::connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qInfo() << "[MapTool::run] error";
        emit result->mapList(mapList, false, QString("QProcessError %1").arg(error));
    });

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int exitCode, QProcess::ExitStatus exitStatus) {

        const char UNIT_SEPARATOR = '\x1f';
        process->setReadChannel(QProcess::StandardOutput);
        while (process->bytesAvailable() > 0)
        {
            QByteArray line = process->readLine();
            while (!line.isEmpty() && (line.endsWith('\r') || line.endsWith('\n')))
            {
                line = line.mid(0, line.size() - 1);
            }
            QStringList mapDetails = QString(line).split(UNIT_SEPARATOR);
            try
            {
                MapToolDto dto(mapDetails);
                mapList->append(dto);
            }
            catch (const std::exception & e)
            {
                qWarning() << "[MapTool::run] Unable to parse MapTool output:" << e.what();
            }
        }

        process->setReadChannel(QProcess::StandardError);
        while (process->bytesAvailable() > 0)
        {
            qInfo() << "[MapTool::run]" << process->readLine();
        }

        emit result->mapList(mapList, true, QString("exitCode=%1, exitStatus=%2").arg(exitCode, exitStatus));
    });

    //QObject::connect(&process, &QProcess::stateChanged, [](QProcess::ProcessState newState) {
    //});

    //QObject::connect(process, &QProcess::readyReadStandardError, [=]() {
    //});

    //QObject::connect(process, &QProcess::readyReadStandardOutput, [=]() {
    //});

    qInfo() << "[MapTool::run]" << mapToolExePath << arguments;
    process->start(mapToolExePath, arguments);
    return result;
}

MapListSignal* MapTool::listMap(QString gamePath, QString mapName)
{
    return run(m_mapToolExePath, gamePath, QString(), mapName + "$", true, QString(), QString(), 0, QString());
}

MapListSignal* MapTool::listMapsInstalled(QString gamePath, bool doCrc)
{
    return run(m_mapToolExePath, gamePath, QString(), QString(), doCrc, QString(), QString(), 0, m_cacheDirectory);
}

MapListSignal* MapTool::listMapsInArchive(QString hpiFile, bool doCrc)
{
    QFileInfo hpiFileInfo(hpiFile);
    return run(m_mapToolExePath, hpiFileInfo.dir().absolutePath(), hpiFileInfo.baseName(), QString(), doCrc, m_cacheDirectory, "mini", 0, QString());
}

MapListSignal* MapTool::generatePreview(QString gamePath, QString mapName, QString previewType, int positionCount)
{
    return run(m_mapToolExePath, gamePath, QString(), mapName + "$", false, m_cacheDirectory, previewType, positionCount, m_cacheDirectory);
}

QString MapTool::getPreviewFilePath(QString mapName, QString previewType, int positionCount)
{
    if (previewType.isEmpty())
    {
        return QString();
    }
    if (0 == previewType.compare("mini", Qt::CaseInsensitive))
    {
        return QDir(m_cacheDirectory).filePath(QString("%1/%2.png").arg(previewType).arg(mapName));
    }
    else
    {
        return QDir(m_cacheDirectory).filePath(QString("%1_%2/%3.png").arg(previewType).arg(positionCount).arg(mapName));
    }
}
