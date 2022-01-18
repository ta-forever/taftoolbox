#pragma once

#include <QtCore/qstring.h>

class ModService
{
public:
    static ModService* initialise();
    static ModService* getInstance();

    QString getModPath(QString featuredMod);

private:
    ModService();

    static ModService* m_instance;
};
