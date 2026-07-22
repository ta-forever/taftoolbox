#include "TafService.h"
#include "DownloadService.h"
#include "NativeTools.h"
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

#include "api/FeaturedModDto.h"
#include "TafEndpoints.h"
#include "preferences/LoginPreferences.h"
#include "preferences/PreferencesService.h"

TafService* TafService::m_tafService = NULL;

TafService::TafService(QObject *parent):
    QObject(parent),
    m_tafLobbyClient(QCoreApplication::applicationName(), QCoreApplication::applicationVersion())
{
    qInfo() << "[TafService::TafService]";
    QObject::connect(&m_tafLobbyClient, &TafLobbyClient::session, [this](quint64 sessionId) {
        LoginPreferences loginPreferences(this);
        QString nativeDir = PreferencesService::getInstance()->getNativeDir();
        QString password = QCryptographicHash::hash(loginPreferences.getPassword().toUtf8(), QCryptographicHash::Sha256).toHex();
        QString login    = loginPreferences.getUserName();

        QProcess* uidProcess = new QProcess(this);
        QObject::connect(uidProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, uidProcess, sessionId, login, password](int, QProcess::ExitStatus) {
                QString uid = QString::fromUtf8(uidProcess->readAll()).trimmed();
                if (uid.isEmpty()) uid = "0";
                m_tafLobbyClient.sendHello(sessionId, uid, "0.0.0.0", login, password);
                uidProcess->deleteLater();
            });
        QObject::connect(uidProcess, &QProcess::errorOccurred,
            [this, uidProcess, sessionId, login, password](QProcess::ProcessError) {
                qWarning() << "[TafService] faf-uid failed to start";
                m_tafLobbyClient.sendHello(sessionId, "0", "0.0.0.0", login, password);
                uidProcess->deleteLater();
            });
        uidProcess->start(nativeDir + "/" + NativeTools::exeName("faf-uid"), QStringList() << QString::number(sessionId));
    });
}

TafService* TafService::initialise(QObject* parent)
{
    return m_tafService = new TafService(parent);
}

TafService* TafService::getInstance()
{
    return m_tafService;
}

TafLobbyClient* TafService::getTafLobbyClient()
{
    return &m_tafLobbyClient;
}

void TafService::setTafEndpoints(const TafEndpoints& tafEndpoints)
{
    m_tafEndpoints = tafEndpoints;
}

void TafService::getDfcConfig(QString dfcConfigUrl, const std::function<void(QSharedPointer<DtoTableModel<TafEndpoints> >)>& callback)
{
    return request<TafEndpoints>(dfcConfigUrl, "endpoints", callback);
}

void TafService::getFeaturedMods(const std::function<void(QSharedPointer<DtoTableModel<FeaturedModDto> >)>& callback)
{
    QUrl url(m_tafEndpoints.apiUrl);
    url.setPath("/data/featuredMod");
    return request<FeaturedModDto>(url, "data", callback);
}
