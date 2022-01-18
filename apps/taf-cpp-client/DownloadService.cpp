#include "DownloadService.h"

#include <QtCore/qfile.h>

DownloadService*DownloadService::m_downloadService = NULL;


// https://stackoverflow.com/questions/41857883/qsslsocket-cannot-call-unresolved-function
// https://indy.fulgan.com/SSL/Archive/

DownloadService* DownloadService::getInstance()
{
    if (m_downloadService == NULL)
    {
        qInfo() << "[DownloadService::getInstance] Creating QNetworkAccessManager";
        qInfo() << "[DownloadService::getInstance] sslLibraryBuildVersion=" << QSslSocket::sslLibraryBuildVersionString();
        m_downloadService = new DownloadService();
        //QObject::connect(
        //    m_downloadService->getNetworkAccessManager(), &QNetworkAccessManager::sslErrors,
        //    m_downloadService, &DownloadService::ignoreSslErrors);
    }
    return m_downloadService;
}

QNetworkAccessManager* DownloadService::getNetworkAccessManager()
{
    return &m_networkAccessManager;
}

bool DownloadService::isInProgress(QString destination)
{
    return m_inProgressCallbacks.contains(destination);
}

void DownloadService::downloadFile(QUrl url, QString destination, std::function<void(QString, QNetworkReply::NetworkError)> callback)
{
    auto & callbackList = m_inProgressCallbacks[destination];
    callbackList.append(callback);
    if (callbackList.size() > 1)
    {
        return;
    }

    QNetworkRequest req(url);
    req.setMaximumRedirectsAllowed(3);
    QNetworkReply* reply = DownloadService::getInstance()->getNetworkAccessManager()->get(req);

    QObject::connect(reply, &QNetworkReply::finished, [this, destination, &callbackList, reply]() {
        QByteArray data = reply->readAll();
        qInfo() << "[DownloadService::getPreview] received bytes:" << data.size();
        for (QByteArray header : reply->rawHeaderList())
        {
            qInfo() << "[DownloadService::getPreview] header item:" << header;
        }
        {
            QFile fp(destination);
            fp.open(QIODevice::WriteOnly);
            fp.write(data);
        }
        for (auto & callback : callbackList)
        {
            callback(destination, reply->error());
        }
        m_inProgressCallbacks.remove(destination);
        reply->deleteLater();
    });

    return;
}

void DownloadService::ignoreSslErrors(QNetworkReply* reply, const QList<QSslError> &)
{
    qWarning() << "[DownloadService::ignoreSslErrors]";
    reply->ignoreSslErrors();
}

