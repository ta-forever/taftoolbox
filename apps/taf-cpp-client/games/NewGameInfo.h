#pragma once

#include <QtCore/qstring.h>

class NewGameInfo
{
public:
    NewGameInfo(QString title, QString password, QString mod, QString mapname, QString visibility, int replayDelaySeconds, QString ratingType);

    QString title;
    QString password;
    QString mod;
    QString mapname;
    QString visibility;
    int replayDelaySeconds;
    QString ratingType;
};

