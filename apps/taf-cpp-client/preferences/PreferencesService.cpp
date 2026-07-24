#include "PreferencesService.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>

PreferencesService* PreferencesService::m_instance = NULL;

PreferencesService::PreferencesService(QObject* parent) :
    m_settings(QSettings::IniFormat, QSettings::UserScope, "TA Forever", "taf-cpp-client")
{ }

PreferencesService* PreferencesService::initialise(QObject* parent)
{
    m_instance = new PreferencesService(parent);
    return m_instance;
}

PreferencesService* PreferencesService::getInstance()
{
    return m_instance;
}

QString PreferencesService::getModPath(QString featuredMod)
{
    m_settings.beginGroup("mods");
    QString path = m_settings.value(featuredMod.toLower()).toString();
    m_settings.endGroup();
    return path;
}

void PreferencesService::setModPath(QString featuredMod, QString path)
{
    m_settings.beginGroup("mods");
    m_settings.setValue(featuredMod.toLower(), path);
    m_settings.endGroup();
}

QString PreferencesService::getNativeDir()
{
    QString stored = m_settings.value("nativeDir").toString();
    if (!stored.isEmpty())
    {
        return stored;
    }
    // default: the exe normally lives in <natives>/bin, but the natives base
    // dir (faf-uid, faf-ice-adapter.jar, bin/) is its parent
    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.dirName() == "bin")
    {
        dir.cdUp();
    }
    return dir.absolutePath();
}

void PreferencesService::setNativeDir(QString path)
{
    m_settings.setValue("nativeDir", path);
}

QString PreferencesService::getLogDir()
{
    return m_settings.value(
        "logDir",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs"
    ).toString();
}
