#include "LoginDialog.h"

#include "taflib/Logger.h"

#include <QtCore/qsettings.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

LoginDialog::LoginDialog(QWidget* parent) :
    QDialog(parent),
    m_ui(new Ui::LoginDialog()),
    m_loginPreferencesModel(QSharedPointer<LoginPreferences>(new LoginPreferences(this)))
{
    qInfo() << "[LoginDialog::LoginDialog]";
    m_ui->setupUi(this);
    m_ui->extraOptionsFrame->hide();
    initLoginPreferences();

    QFile css(":/LoginDialog.css");
    css.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = QString(css.readAll()).replace("%THEMEPATH%", ":/res/images");
    this->setStyleSheet(data);
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::on_loginButton_clicked()
{
    const TafEndpoints* endpoints = m_endpoints->getDtoById(m_ui->environmentBox->currentText());
    if (endpoints)
    {
        emit loginRequested(
            m_ui->userNameField->text(),
            m_ui->passwordField->text(),
            *endpoints);
    }
}

void LoginDialog::on_environmentBox_currentIndexChanged(QString name)
{
    qInfo() << "[LoginDialog::on_environmentBox_currentIndexChanged]" << name;
    const TafEndpoints* endpoints = m_endpoints->getDtoById(name);
    int row = m_endpoints->getRowById(name);
    if (row >= 0)
    {
        m_endpointsMapper.setCurrentIndex(row);
    }
    else
    {
        qWarning() << "[LoginDialog::on_environmentBox_currentIndexChanged] NULL model!";
    }
}

void LoginDialog::on_extraOptionsToggle_clicked()
{
    if (m_ui->extraOptionsFrame->isHidden())
    {
        m_ui->extraOptionsFrame->show();
    }
    else
    {
        m_ui->extraOptionsFrame->hide();
    }
}

void LoginDialog::setTafEndpoints(QSharedPointer<DtoTableModel<TafEndpoints> > endpointsTableModel)
{
    m_endpoints = endpointsTableModel;
    m_endpointsMapper.setModel(endpointsTableModel.data());
    m_endpointsMapper.setOrientation(Qt::Horizontal);
    m_endpointsMapper.addMapping(m_ui->serverHostField, int(TafEndpoints::Fields::LobbyHostStr));
    m_endpointsMapper.addMapping(m_ui->serverPortField, int(TafEndpoints::Fields::LobbyPortInt));
    m_endpointsMapper.addMapping(m_ui->ircServerHostField, int(TafEndpoints::Fields::IrcHostStr));
    m_endpointsMapper.addMapping(m_ui->ircServerPortField, int(TafEndpoints::Fields::IrcPortInt));
    m_endpointsMapper.addMapping(m_ui->replayServerHostField, int(TafEndpoints::Fields::LiveReplayHostStr));
    m_endpointsMapper.addMapping(m_ui->replayServerPortField, int(TafEndpoints::Fields::LiveReplayPortInt));
    m_endpointsMapper.addMapping(m_ui->apiURLField, int(TafEndpoints::Fields::ApiUrlStr));
    m_endpointsMapper.setCurrentIndex(0);

    m_ui->environmentBox->clear();
    for (int row = 0; row < endpointsTableModel->rowCount(); ++row)
    {
        const TafEndpoints* endpoints = endpointsTableModel->getDtoByRow(row);
        m_ui->environmentBox->addItem(endpoints->name);
    }

    if (m_ui->rememberCheckbox->isChecked())
    {
        const TafEndpoints* selectedEndpoints = m_endpoints->getDtoById(m_ui->environmentBox->currentText());
        if (selectedEndpoints)
        {
            emit loginRequested(m_ui->userNameField->text(), m_ui->passwordField->text(), *selectedEndpoints);
        }
    }
}

void LoginDialog::initLoginPreferences()
{
    if (!m_loginPreferencesModel.getLoginPreferences()->getRemember())
    {
        m_loginPreferencesModel.getLoginPreferences()->setPassword("");
    }

    m_loginPreferencesMapper.setModel(&m_loginPreferencesModel);
    m_loginPreferencesMapper.setOrientation(Qt::Vertical);
    for (int n = 0; n < m_loginPreferencesModel.rowCount(); ++n)
    {
        QString name = m_loginPreferencesModel.headerData(n, Qt::Vertical, Qt::DisplayRole).toString();
        if (name == "remember") {
            m_loginPreferencesMapper.addMapping(m_ui->rememberCheckbox, n);
        }
        else if (name == "user_name") {
            m_loginPreferencesMapper.addMapping(m_ui->userNameField, n);
        }
        else if (name == "password") {
            m_loginPreferencesMapper.addMapping(m_ui->passwordField, n);
        }
    }
    m_loginPreferencesMapper.toFirst();
}
