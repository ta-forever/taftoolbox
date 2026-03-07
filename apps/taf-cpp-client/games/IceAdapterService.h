#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonvalue.h>
#include <QtNetwork/qtcpsocket.h>

class IceAdapterService : public QObject
{
    Q_OBJECT

public:
    static IceAdapterService* initialise(QObject* parent);
    static IceAdapterService* getInstance();
    static int findFreePort();

    // Launch faf-ice-adapter.jar and connect to its RPC port.
    void start(int playerId, int gameUid, QString playerName, int rpcPort, int gpgNetPort);
    void stop();

    QString getLogPath() const { return m_iceAdapterLogPath; }

    // JSON-RPC fire-and-forget calls (safe to call before rpcConnected — queued internally)
    void hostGame(QString mapName);
    void joinGame(QString login, int remotePlayerId);
    void connectToPeer(QString login, int remotePlayerId, bool offer);
    void disconnectFromPeer(int remotePlayerId);
    void iceMsg(int remotePlayerId, QJsonValue msg);
    void setIceServers(QJsonArray iceServers);
    void setLobbyInitMode(QString mode);

signals:
    void rpcConnected();
    void onIceMsg(int remotePlayerId, QJsonValue msg);
    void onConnected(int remotePlayerId, bool connected);
    void gpgNetMessageReceived(QString command, QJsonArray args);
    void gameConnectionLost();

private slots:
    void onSocketReadyRead();
    void onSocketConnected();
    void tryConnect();

private:
    explicit IceAdapterService(QObject* parent);
    void sendRpc(QString method, QJsonArray params);
    void handleNotification(QString method, QJsonArray params);

    static IceAdapterService* m_instance;

    QProcess*  m_process = nullptr;
    QTcpSocket m_socket;
    QTimer     m_connectTimer;
    int        m_rpcPort      = 0;
    int        m_rpcIdCounter = 0;
    QByteArray m_readBuf;
    QString    m_iceAdapterLogPath;
};
