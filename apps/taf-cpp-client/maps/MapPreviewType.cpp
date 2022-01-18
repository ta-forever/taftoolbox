#include "MapPreviewType.h"

QString getMapPreviewTypeName(MapPreviewType previewType)
{
    switch (previewType)
    {
    case MapPreviewType::Mini: return QString("mini");
    case MapPreviewType::Positions: return QString("positions");
    case MapPreviewType::Mexes: return QString("mexes");
    case MapPreviewType::Geos: return QString("geos");
    case MapPreviewType::Rocks: return QString("rocks");
    case MapPreviewType::Trees: return QString("trees");
    default: return QString();
    }
}
