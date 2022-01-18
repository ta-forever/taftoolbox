#include "TafService.h"
#include "DownloadService.h"
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>

#include "api/FeaturedModDto.h"
#include "TafEndpoints.h"
#include "preferences/LoginPreferences.h"

TafService* TafService::m_tafService = NULL;

TafService::TafService(QObject *parent):
    QObject(parent),
    m_tafLobbyClient(QCoreApplication::applicationName(), QCoreApplication::applicationVersion())
{
    qInfo() << "[TafService::TafService]";
    QObject::connect(&m_tafLobbyClient, &TafLobbyClient::session, [this](quint64 sessionId) {
        LoginPreferences loginPreferences(this);
        QString uid = "0"; // TafHwIdGenerator("C:/Program Files/TA Forever Client/natives/faf-uid.exe").get(sessionId);
        QString password = QCryptographicHash::hash(loginPreferences.getPassword().toUtf8(), QCryptographicHash::Sha256).toHex();
        m_tafLobbyClient.sendHello(sessionId, uid, "0.0.0.0", loginPreferences.getUserName(), password);
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
