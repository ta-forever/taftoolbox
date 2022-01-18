#pragma once

#include <QtCore/qsettings.h>



class PreferencesService
{
public:
    PreferencesService(QObject* parent);

    static PreferencesService* initialise(QObject* parent);
    static PreferencesService* getInstance();



private:
    static PreferencesService* m_instance;

    QSettings m_settings;
};
