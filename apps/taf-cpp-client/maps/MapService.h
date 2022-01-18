#pragma once

#include "MapPreviewType.h"

#include "ta/MapToolDto.h"

#include <QtCore/qobject.h>
#include <QtCore/qfuture.h>
#include <functional>

class MapService: public QObject
{
    Q_OBJECT

private:
    const char* MAP_PREVIEW_DOWNLOAD_FORMAT = "{vault_url}/maps/previews/{type}/{map_name}.png";

public:

    static MapService* initialise(QObject* parent);
    static MapService* getInstance();
    MapService(QObject* parent);

    QString getPreviewCacheFilePath(QString mapName, MapPreviewType previewType, int positionCount);
    bool isPreviewAvailable(QString mapName, MapPreviewType previewType, int positionCount);
    void getPreview(QString mapName, MapPreviewType previewType, int positionCount, QString featuredMod, std::function<void(QString)> callback);
    void getInstalledMaps(QString featuredMod, bool withCrc, std::function<void(const QList<MapToolDto> &)>);

private:
    static MapService* m_mapService;
    QMap<QString, QList<MapToolDto> > m_installedMaps;

    void _getPreviewFromCache(QString mapName, MapPreviewType previewType, int positionCount, std::function<void(QString)> callback);
    void _getPreviewFromServer(QString mapName, MapPreviewType previewType, int positionCount, std::function<void(QString)> callback);
    void _getPreviewFromGamePath(QString mapName, MapPreviewType previewType, int positionCount, QString featuredMod,
        std::function<void(QString)> callback);
};
