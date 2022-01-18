#pragma once

#include <QtCore/qjsonobject.h>
#include <QtWidgets/qdatawidgetmapper.h>

#include "ui_LoginDialog.h"

#include "TafEndpoints.h"
#include "preferences/LoginPreferences.h"

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    LoginDialog(QWidget* parent = 0);
    ~LoginDialog();

    void setTafEndpoints(QSharedPointer<DtoTableModel<TafEndpoints> >);

signals:
    void loginRequested(QString userName, QString password, const TafEndpoints &endpoints);

private slots:
    void on_loginButton_clicked();
    void on_environmentBox_currentIndexChanged(QString);
    void on_extraOptionsToggle_clicked();

private:
    QSharedPointer<Ui::LoginDialog> m_ui;
    QSharedPointer<DtoTableModel<TafEndpoints> > m_endpoints;
    QDataWidgetMapper m_endpointsMapper;

    LoginPreferencesModel m_loginPreferencesModel;
    QDataWidgetMapper m_loginPreferencesMapper;

    void initLoginPreferences();
};
