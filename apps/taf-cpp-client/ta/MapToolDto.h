#pragma once

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include "DtoTableModel.h"

class MapToolDto
{
public:
    enum class Fields
    {
        Dto = 0,
        NameStr,
        ArchiveStr,
        CrcStr,
        DescriptionStr,
        SizeStr,
        PlayersInt,
        WindStr,
        TidalStr,
        GravityStr,
        _COLUMN_COUNT
    };

    typedef QString IdType;

    MapToolDto();
    MapToolDto(const QStringList& mapDetails);
    QVariant get(Fields f) const;
    IdType id() const;

    QString name;
    QString archive;
    QString crc;
    QString description;
    QString size;
    int players;
    QString wind;
    QString tidal;
    QString gravity;
};

Q_DECLARE_METATYPE(MapToolDto);
Q_DECLARE_METATYPE(QSharedPointer<DtoTableModel<MapToolDto> >);
