#pragma once

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

enum class MapPreviewType
{
    None,
    Mini,
    Positions,
    Mexes,
    Geos,
    Rocks,
    Trees
};

Q_DECLARE_METATYPE(MapPreviewType);

QString getMapPreviewTypeName(MapPreviewType previewType);
