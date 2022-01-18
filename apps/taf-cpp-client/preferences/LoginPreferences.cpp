#include "LoginPreferences.h"

static const QMap<QString, QVariant> LOGIN_PREFERENCES_DEFAULTS({
    std::make_pair<QString, QVariant>(QString("remember"), QVariant("false")),
    std::make_pair<QString, QVariant>(QString("user_name"), QVariant()),
    std::make_pair<QString, QVariant>(QString("password"), QVariant())
    });

static const QVector<QString> LOGIN_PREFERENCES_KEYS(LOGIN_PREFERENCES_DEFAULTS.keys().toVector());

LoginPreferences::LoginPreferences(QObject* parent) :
    m_settings(new QSettings(parent))
{
    m_settings->beginGroup("login");

    for (auto it = LOGIN_PREFERENCES_DEFAULTS.begin(); it != LOGIN_PREFERENCES_DEFAULTS.end(); ++it)
    {
        if (!m_settings->contains(it.key()))
        {
            m_settings->setValue(it.key(), it.value());
        }
    }
}

QSettings & LoginPreferences::getSettings()
{
    return *m_settings.data();
}

const QSettings& LoginPreferences::getSettings() const
{
    return *m_settings.data();
}

bool LoginPreferences::getRemember() const
{
    return m_settings->value("remember").toBool();
}

void LoginPreferences::setRemember(bool remember)
{
    m_settings->setValue("remember", remember);
}

QString LoginPreferences::getUserName() const
{
    return m_settings->value("user_name").toString();
}

void LoginPreferences::setUserName(QString userName)
{
    m_settings->setValue("user_name", userName);
}

QString LoginPreferences::getPassword() const
{
    return m_settings->value("password").toString();
}

void LoginPreferences::setPassword(QString password)
{
    m_settings->setValue("password", password);
}

LoginPreferencesModel::LoginPreferencesModel(QSharedPointer<LoginPreferences> loginPreferences):
    m_loginPreferences(loginPreferences)
{ }

QSharedPointer<LoginPreferences> LoginPreferencesModel::getLoginPreferences()
{
    return m_loginPreferences;
}

int LoginPreferencesModel::rowCount(const QModelIndex& parent) const
{
    return LOGIN_PREFERENCES_DEFAULTS.size();
}

QVariant LoginPreferencesModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::EditRole && index.row() >=0 && index.row() < LOGIN_PREFERENCES_KEYS.size())
    {
        QString key = LOGIN_PREFERENCES_KEYS[index.row()];
        return m_loginPreferences->getSettings().value(key);
    }
    return QVariant();
}

QVariant LoginPreferencesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Vertical && section >= 0 && section < LOGIN_PREFERENCES_KEYS.size())
    {
        return LOGIN_PREFERENCES_KEYS[section];
    }
    return QVariant();
}

bool LoginPreferencesModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role == Qt::EditRole && index.row() >= 0 && index.row() < LOGIN_PREFERENCES_KEYS.size())
    {
        QString key = LOGIN_PREFERENCES_KEYS[index.row()];
        m_loginPreferences->getSettings().setValue(key, value);
        emit dataChanged(index, index);
        return true;
    }
    else
    {
        return false;
    }
}

Qt::ItemFlags LoginPreferencesModel::flags(const QModelIndex& index) const
{
    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}
