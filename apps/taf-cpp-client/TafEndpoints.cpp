#include <QtCore/qjsonobject.h>
#include <QtCore/qurl.h>

#include "TafEndpoints.h"
#include "taflib/Logger.h"

TafEndpoints::TafEndpoints()
{ }

TafEndpoints::TafEndpoints(const QJsonObject& json)
{
    name = json["name"].toString();
    lobbyHost = json["lobby"].toObject()["host"].toString();
    lobbyPort = json["lobby"].toObject()["port"].toInt();
    ircHost = json["irc"].toObject()["host"].toString();
    ircPort = json["irc"].toObject()["port"].toInt();
    liveReplayHost = json["liveReplay"].toObject()["host"].toString();
    liveReplayPort = json["liveReplay"].toObject()["port"].toInt();
    apiUrl = json["api"].toObject()["url"].toString();
}

QVariant TafEndpoints::get(Fields f) const
{
    switch (f)
    {
    case Fields::Dto: return QVariant::fromValue(*this);
    case Fields::NameStr: return QVariant::fromValue(name);
    case Fields::LobbyHostStr: return QVariant::fromValue(lobbyHost);
    case Fields::LobbyPortInt: return QVariant::fromValue(lobbyPort);
    case Fields::IrcHostStr: return QVariant::fromValue(ircHost);
    case Fields::IrcPortInt: return QVariant::fromValue(ircPort);
    case Fields::LiveReplayHostStr: return QVariant::fromValue(liveReplayHost);
    case Fields::LiveReplayPortInt: return QVariant::fromValue(liveReplayPort);
    case Fields::ApiUrlStr: return QVariant::fromValue(apiUrl);
    default: return QVariant();
    }
}

TafEndpoints::IdType TafEndpoints::id() const
{
    return name;
}

//TafEndpointsItemModel::TafEndpointsItemModel(QSharedPointer<TafEndpoints> tafEndpoints) :
//    m_tafEndpoints(tafEndpoints)
//{ }
//
//QSharedPointer<TafEndpoints> TafEndpointsItemModel::getTafEndpoints()
//{
//    return m_tafEndpoints;
//}
//
//QVariant TafEndpointsItemModel::data(const QModelIndex& index, int role) const
//{
//    if (!index.isValid() || index.row() >= 8)
//    {
//        return QVariant();
//    }
//
//    switch (role)
//    {
//    case Qt::DisplayRole:
//    case Qt::EditRole:
//        switch (index.row())
//        {
//        case 0: return QVariant(m_tafEndpoints->name);
//        case 1: return QVariant(m_tafEndpoints->lobbyHost);
//        case 2: return QVariant(m_tafEndpoints->lobbyPort);
//        case 3: return QVariant(m_tafEndpoints->ircHost);
//        case 4: return QVariant(m_tafEndpoints->ircPort);
//        case 5: return QVariant(m_tafEndpoints->liveReplayHost);
//        case 6: return QVariant(m_tafEndpoints->liveReplayPort);
//        case 7: return QVariant(m_tafEndpoints->apiUrl);
//        }
//    }
//
//    return QVariant();
//}
//
//int TafEndpointsItemModel::rowCount(const QModelIndex& parent) const
//{
//    return 8;
//}
//
//bool TafEndpointsItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
//{
//    if (index.isValid() && role == Qt::EditRole)
//    {
//        switch (index.row())
//        {
//        case 0: m_tafEndpoints->name = value.toString(); break;
//        case 1: m_tafEndpoints->lobbyHost = value.toString(); break;
//        case 2: m_tafEndpoints->lobbyPort = value.toInt(); break;
//        case 3: m_tafEndpoints->ircHost = value.toString(); break;
//        case 4: m_tafEndpoints->ircPort = value.toInt(); break;
//        case 5: m_tafEndpoints->liveReplayHost = value.toString(); break;
//        case 6: m_tafEndpoints->liveReplayPort = value.toInt(); break;
//        case 7: m_tafEndpoints->apiUrl = value.toString(); break;
//        }
//        emit dataChanged(index, index);
//        return true;
//    }
//    else
//    {
//        return false;
//    }
//}
//
//Qt::ItemFlags TafEndpointsItemModel::flags(const QModelIndex& index) const
//{
//    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
//}
