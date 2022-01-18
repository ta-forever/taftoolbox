#include "NewGameInfo.h"

NewGameInfo::NewGameInfo(
    QString title, QString password, QString mod, QString mapname,
    QString visibility, int replayDelaySeconds, QString ratingType):
    title(title),
    password(password),
    mod(mod),
    mapname(mapname),
    visibility(visibility),
    replayDelaySeconds(replayDelaySeconds),
    ratingType(ratingType)
{ }
