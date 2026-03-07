#pragma once

#include <QtCore/qsettings.h>
#include <QtCore/qstring.h>

class PreferencesService
{
public:
    PreferencesService(QObject* parent);

    static PreferencesService* initialise(QObject* parent);
    static PreferencesService* getInstance();

    QString getModPath(QString featuredMod);
    void    setModPath(QString featuredMod, QString path);

    QString getNativeDir();
    void    setNativeDir(QString path);

    QString getLogDir();

private:
    static PreferencesService* m_instance;

    QSettings m_settings;
};
