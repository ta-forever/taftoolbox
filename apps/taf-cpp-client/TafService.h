#pragma once

#include <QtCore/qlist.h>
#include <QtCore/qurl.h>
#include <QtNetwork/qnetworkreply.h>

#include "TafEndpoints.h"
#include "tafclient/TafLobbyClient.h"
#include "api/FeaturedModDto.h"

#include <functional>

class TafService: public QObject
{
    Q_OBJECT
private:
    static TafService* m_tafService;

public:
    static TafService* initialise(QObject* parent);
    static TafService* getInstance();

    TafService(QObject *parent);
    TafLobbyClient* getTafLobbyClient();
    void setTafEndpoints(const TafEndpoints& tafEndpoints);

    void getDfcConfig(QString dfcConfigUrl, const std::function<void(QSharedPointer<DtoTableModel<TafEndpoints> >)> &callback);
    void getFeaturedMods(const std::function<void(QSharedPointer<DtoTableModel<FeaturedModDto> >)> &callback);

private:
    template<typename DtoType>
    void request(QUrl url, QString replyJsonRoot, const std::function<void(QSharedPointer<DtoTableModel<DtoType> >)> &callback)
    {
        qInfo() << "[TafService::request]" << url;
        QNetworkReply* reply = DownloadService::getInstance()->getNetworkAccessManager()->get(QNetworkRequest(url));
        QObject::connect(reply, &QNetworkReply::finished, [this, reply, replyJsonRoot, callback]()
        {
            this->handleNetworkReply<DtoType>(reply, replyJsonRoot, callback);
        });
    }

    template<typename DtoType>
    void handleNetworkReply(QNetworkReply* reply, QString replyJsonRoot, const std::function<void(QSharedPointer<DtoTableModel<DtoType> >)>& callback)
    {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        QSharedPointer<DtoTableModel<DtoType> > model(new DtoTableModel<DtoType>(this));
        if (doc.object().contains("errors"))
        {
            qWarning() << "TafService::parseNetworkReply]" << doc;
        }

        for (const QJsonValue& item : doc.object()[replyJsonRoot].toArray())
        {
            model->append(DtoType(item.toObject()));
        }
        callback(model);
        reply->deleteLater();
    }

    TafLobbyClient m_tafLobbyClient;
    TafEndpoints m_tafEndpoints;
};
