#include "ModService.h"

#include "taflib/Logger.h"

ModService* ModService::m_instance = 0;

ModService* ModService::initialise()
{
    return m_instance = new ModService();
}

ModService* ModService::getInstance()
{
    return m_instance;
}

ModService::ModService()
{
}

QString ModService::getModPath(QString featuredMod)
{
    if (0 == featuredMod.compare("tacc", Qt::CaseInsensitive))
    {
        return "d:/games/TA";
    }
    else if (0 == featuredMod.compare("tavmod", Qt::CaseInsensitive))
    {
        return "d:/games/ProTA";
    }
    else if (0 == featuredMod.compare("taesc", Qt::CaseInsensitive))
    {
        return "d:/games/TAESC";
    }
    else
    {
        qInfo() << "[ModService::getModPath] unknown mod";
        return QString();
    }
}
