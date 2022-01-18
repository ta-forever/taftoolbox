#include "FeaturedModDto.h"
#include <QtCore/qvariant.h>

FeaturedModDto::FeaturedModDto()
{
}

FeaturedModDto::FeaturedModDto(const QJsonObject& jsonObject)
{
    QString type = jsonObject.value("type").toString();
    if (type != "featuredMod")
    {
        throw std::runtime_error(QString("Expected type=FeaturedMod, got type=%1")
            .arg(type).toStdString());
    }

    const QJsonObject& attributes = jsonObject.value("attributes").toObject();
    technicalName = attributes.value("technicalName").toString();
    displayName = attributes.value("displayName").toString();
    description = attributes.value("description").toString();
    order = attributes.value("order").toInt();
    visible = attributes.value("visible").toBool();
}

QVariant FeaturedModDto::get(Fields f) const
{
    switch (f)
    {
    case Fields::Dto: return QVariant::fromValue(*this);
    case Fields::TechnicalNameStr: return QVariant::fromValue(technicalName);
    case Fields::DisplayNameStr: return QVariant::fromValue(displayName);
    case Fields::DescriptionStr: return QVariant::fromValue(description);
    case Fields::OrderInt: return QVariant::fromValue(order);
    case Fields::VisibleBool: return QVariant::fromValue(visible);
    }
    return QVariant();
}

FeaturedModDto::IdType FeaturedModDto::id() const
{
    return technicalName;
}
