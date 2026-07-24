#include "ModService.h"

#include "preferences/PreferencesService.h"
#include "taflib/Logger.h"

#include <QtCore/qdir.h>
#include <QtWidgets/qfiledialog.h>

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

QString ModService::getModPath(QString featuredMod, bool promptIfMissing)
{
    QString path = PreferencesService::getInstance()->getModPath(featuredMod);
    if (!path.isEmpty() && QDir(path).exists())
        return path;

    if (!promptIfMissing)
        return QString();

    qInfo() << "[ModService::getModPath] no saved path for" << featuredMod << "- prompting user";
    path = QFileDialog::getExistingDirectory(
        nullptr,
        QString("Locate %1 game installation folder (the folder containing TotalA.exe)").arg(featuredMod));
    if (!path.isEmpty())
        PreferencesService::getInstance()->setModPath(featuredMod, path);
    return path;
}
