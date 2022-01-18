#pragma once

#include "MapToolDto.h"

#include <QtCore/qstring.h>

#include <functional>

class MapListSignal : public QObject
{
    Q_OBJECT

signals:
    void mapList(QSharedPointer<QList<MapToolDto> > mapList, bool ok, QString reason);
};

class MapTool
{
public:
    static MapTool* initialise(QString mapToolExePath, QString cacheDirectory);
    static MapTool* getInstance();

    MapListSignal* listMap(QString gamePath, QString mapName);
    MapListSignal* listMapsInstalled(QString gamePath, bool doCrc);
    MapListSignal* listMapsInArchive(QString hpiFile, bool doCrc);

    MapListSignal* generatePreview(QString gamePath, QString mapName, QString previewType, int positionCount);
    QString getPreviewFilePath(QString mapName, QString previewType, int positionCount);

private:
    MapTool(QString mapToolExePath, QString cacheDirectory);

    static MapTool* m_instance;
    const QString m_mapToolExePath;
    const QString m_cacheDirectory;

    static MapListSignal* run(
        QString mapToolExePath, QString gamePath, QString hpiSpecs, QString mapName, bool doCrc,
        QString previewCacheDirectory, QString previewType, int maxPositions, QString featuresCacheDirectory);
};
