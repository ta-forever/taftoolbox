#include "PreferencesService.h"

PreferencesService* PreferencesService::m_instance = NULL;

PreferencesService::PreferencesService(QObject* parent) :
    m_settings(parent)
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

