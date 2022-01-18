#pragma once

#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>
#include <functional>

class DownloadService: public QObject
{
    Q_OBJECT

private:
    static DownloadService *m_downloadService;
    QNetworkAccessManager m_networkAccessManager;
    QMap<QString, QList<std::function<void(QString, QNetworkReply::NetworkError)> > > m_inProgressCallbacks;

public:
    static DownloadService* getInstance();

    QNetworkAccessManager* getNetworkAccessManager();

    bool isInProgress(QString destination);
    void downloadFile(QUrl url, QString destination, std::function<void(QString, QNetworkReply::NetworkError)> callback);

private slots:
    void ignoreSslErrors(QNetworkReply* reply, const QList<QSslError> &);
};
