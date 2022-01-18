#pragma once

#include <QtCore/qstring.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qsettings.h>
#include <QtCore/qsharedpointer.h>

class LoginPreferences
{
public:
    LoginPreferences(QObject* parent);

    bool getRemember() const;
    void setRemember(bool);

    QString getUserName() const;
    void setUserName(QString);

    QString getPassword() const;
    void setPassword(QString);

    QSettings& getSettings();
    const QSettings& getSettings() const;

private:
    QSharedPointer<QSettings> m_settings;
};

class LoginPreferencesModel : public QAbstractListModel
{
public:
    LoginPreferencesModel(QSharedPointer<LoginPreferences> loginPreferences);
    QSharedPointer<LoginPreferences> getLoginPreferences();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    QSharedPointer<LoginPreferences> m_loginPreferences;
};