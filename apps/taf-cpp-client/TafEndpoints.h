#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qsharedpointer.h>
#include "DtoTableModel.h"

class TafEndpoints
{
public:
    enum class Fields
    {
        Dto = 0,
        NameStr,
        LobbyHostStr,
        LobbyPortInt,
        IrcHostStr,
        IrcPortInt,
        LiveReplayHostStr,
        LiveReplayPortInt,
        ApiUrlStr,
        _COLUMN_COUNT
    };

    typedef QString IdType;

    TafEndpoints();
    TafEndpoints(const QJsonObject& json);
    QVariant get(Fields f) const;
    IdType id() const;

    QString name;
    QString lobbyHost;
    quint16 lobbyPort;
    QString ircHost;
    quint16 ircPort;
    QString liveReplayHost;
    quint16 liveReplayPort;
    QString apiUrl;
};

Q_DECLARE_METATYPE(TafEndpoints);
Q_DECLARE_METATYPE(QSharedPointer<DtoTableModel<TafEndpoints> >);
