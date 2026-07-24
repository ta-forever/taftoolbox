#pragma once

#include <QtCore/qstring.h>

class ModService
{
public:
    static ModService* initialise();
    static ModService* getInstance();

    // promptIfMissing: only pass true on a deliberate user action (launching a
    // game, hosting) — passive callers (map preview rendering) must not pop a
    // folder picker at the user
    QString getModPath(QString featuredMod, bool promptIfMissing = true);

private:
    ModService();

    static ModService* m_instance;
};
