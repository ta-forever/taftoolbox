#include "MapToolDto.h"
#include <QtCore/qvariant.h>

MapToolDto::MapToolDto()
{
}

MapToolDto::MapToolDto(const QStringList &mapDetails)
{
    if (mapDetails.size() < 9)
    {
        throw std::runtime_error(QString("Expected 9 map details, got %1")
            .arg(mapDetails.size()).toStdString());
    }

    name = mapDetails[0];
    archive = mapDetails[1];
    crc = mapDetails[2];
    description = mapDetails[3];
    size = mapDetails[4];
    players = mapDetails[5].toInt();
    wind = mapDetails[6];
    tidal = mapDetails[7];
    gravity = mapDetails[8];
}

QVariant MapToolDto::get(Fields f) const
{
    switch (f)
    {
    case Fields::Dto: return QVariant::fromValue(*this);
    case Fields::NameStr: return QVariant::fromValue(name);
    case Fields::ArchiveStr: return QVariant::fromValue(archive);
    case Fields::CrcStr: return QVariant::fromValue(crc);
    case Fields::DescriptionStr: return QVariant::fromValue(description);
    case Fields::SizeStr: return QVariant::fromValue(size);
    case Fields::PlayersInt: return QVariant::fromValue(players);
    case Fields::WindStr: return QVariant::fromValue(wind);
    case Fields::TidalStr: return QVariant::fromValue(tidal);
    case Fields::GravityStr: return QVariant::fromValue(gravity);
    }
    return QVariant();
}

MapToolDto::IdType MapToolDto::id() const
{
    return name;
}
