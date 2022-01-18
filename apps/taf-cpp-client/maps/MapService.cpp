#include "MapService.h"

#include "taflib/Logger.h"
#include "DownloadService.h"
#include "ta/MapTool.h"
#include "mods/ModService.h"

#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qimage.h>

MapService* MapService::m_mapService = NULL;

MapService* MapService::initialise(QObject* parent)
{
    return m_mapService = new MapService(parent);
}

MapService* MapService::getInstance()
{
    return m_mapService;
}

MapService::MapService(QObject* parent) :
    QObject(parent)
{
    qInfo() << "[MapService::MapService]";
}

QString MapService::getPreviewCacheFilePath(QString mapName, MapPreviewType previewType, int positionCount)
{
    return MapTool::getInstance()->getPreviewFilePath(mapName, getMapPreviewTypeName(previewType), positionCount);
}

bool MapService::isPreviewAvailable(QString mapName, MapPreviewType previewType, int positionCount)
{
    QString destination = getPreviewCacheFilePath(mapName, previewType, positionCount);
    return QFile(destination).exists();
}

void MapService::getPreview(QString mapName, MapPreviewType previewType, int positionCount, QString featuredMod, std::function<void(QString)> callback)
{
    QString destination = getPreviewCacheFilePath(mapName, previewType, positionCount);
    if (QFile(destination).exists())
    {
        qInfo() << "[MapService::getPreview] already available at path" << destination;
        callback(destination);
    }
    else {
        _getPreviewFromGamePath(mapName, previewType, positionCount, featuredMod, [=](QString path) {
            if (QFile(destination).exists())
            {
                callback(destination);
            }
            else
            {
                _getPreviewFromServer(mapName, previewType, positionCount, callback);
            }
        });
    }
}

void MapService::getInstalledMaps(QString featuredMod, bool withCrc, std::function<void(const QList<MapToolDto> &)> callback)
{
    auto it = m_installedMaps.find(featuredMod);
    if (it != m_installedMaps.end())
    {
        callback(*it);
        return;
    }

    QString gamePath = ModService::getInstance()->getModPath(featuredMod);
    if (gamePath.isEmpty())
    {
        callback(QList<MapToolDto>());
        return;
    }

    MapListSignal* result = MapTool::getInstance()->listMapsInstalled(gamePath, withCrc);
    QObject::connect(result, &MapListSignal::mapList, [=](QSharedPointer<QList<MapToolDto> > mapList) {
        if (mapList.isNull())
        {
            qInfo() << QString("[MapService::getInstalledMaps] mod %1, path %2, null maps").arg(featuredMod).arg(gamePath);
        }
        else
        {
            qInfo() << QString("[MapService::getInstalledMaps] mod %1, path %2, %3 maps").arg(featuredMod).arg(gamePath).arg(mapList->size());
            callback(*mapList);
            this->m_installedMaps[featuredMod] = *mapList;
            result->deleteLater();
        }
    });
}

void MapService::_getPreviewFromCache(QString mapName, MapPreviewType previewType, int positionCount, std::function<void(QString)> callback)
{
    QString destination = getPreviewCacheFilePath(mapName, previewType, positionCount);
    if (QFile(destination).exists())
    {
        callback(destination);
    }
    else
    {
        callback(QString());
    }
}

void MapService::_getPreviewFromServer(QString mapName, MapPreviewType previewType, int positionCount, std::function<void(QString)> callback)
{
    QString url = QString(MAP_PREVIEW_DOWNLOAD_FORMAT)
        .replace("{vault_url}", "https://content.taforever.com")
        .replace("{type}", getMapPreviewTypeName(previewType))
        .replace("{map_name}", mapName);

    QString destinationPath = getPreviewCacheFilePath(mapName, previewType, positionCount);
    DownloadService::getInstance()->downloadFile(url, destinationPath,
        [=](QString destination, QNetworkReply::NetworkError errorCode) {
        if (errorCode == QNetworkReply::NetworkError::NoError)
        {
            this->_getPreviewFromCache(mapName, previewType, positionCount, callback);
        }
        else
        {
            qWarning() << "[MapService::_getPreviewFromServer] download failed to path" << destination;
            callback(QString());
        }
    });
}

void MapService::_getPreviewFromGamePath(QString mapName, MapPreviewType previewType, int positionCount, QString featuredMod,
    std::function<void(QString)> callback)
{
    QString gamePath = ModService::getInstance()->getModPath(featuredMod);
    if (gamePath.isEmpty())
    {
        callback(QString());
        return;
    }

    if (previewType == MapPreviewType::None)
    {
        callback(QString());
        return;
    }

    MapListSignal* mapListSignal = MapTool::getInstance()->generatePreview(gamePath, mapName, getMapPreviewTypeName(previewType), positionCount);
    QObject::connect(mapListSignal, &MapListSignal::mapList, [=](QSharedPointer<QList<MapToolDto> > mapList) {
        this->_getPreviewFromCache(mapName, previewType, positionCount, callback);
    });
}
