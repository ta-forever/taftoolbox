#pragma once

#include <QtCore/qstring.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include "DtoTableModel.h"

class FeaturedModDto
{
public:
    enum class Fields
    {
        Dto = 0,
        TechnicalNameStr,
        DisplayNameStr,
        DescriptionStr,
        OrderInt,
        VisibleBool,
        _COLUMN_COUNT
    };

    typedef QString IdType;

    FeaturedModDto();
    FeaturedModDto(const QJsonObject& jsonObject);
    QVariant get(Fields f) const;
    IdType id() const;

    QString technicalName;
    QString displayName;
    QString description;
    int order;
    bool visible;
};

Q_DECLARE_METATYPE(FeaturedModDto);
Q_DECLARE_METATYPE(QSharedPointer<DtoTableModel<FeaturedModDto> >);
