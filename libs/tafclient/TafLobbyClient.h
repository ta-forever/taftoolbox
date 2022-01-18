#pragma once

#include "TafLobbyJsonProtocol.h"

#include "QtCore/qdatastream.h"
#include "QtNetwork/qtcpsocket.h"

struct TafLobbyPlayerInfo
{
    TafLobbyPlayerInfo();
    TafLobbyPlayerInfo(const QJsonObject& playerInfo);
    qint64 id;
    QString login;
    QString alias;
    QString avatarUrl;
    QString avatarTooltip;
    QString country;
    QString state;
    int afkSeconds;
    int currentGameUid;
};
Q_DECLARE_METATYPE(TafLobbyPlayerInfo);

class TafLobbyGameInfo
{
public:
    TafLobbyGameInfo();
    TafLobbyGameInfo(const QJsonObject& playerInfo);
    qint64 id;
    QString host;
    QString title;
    QString featuredMod;
    QString mapName;
    QString mapFilePath;    // "totala2.hpi/SHERWOOD/ead82fc5"
    QString gameType;
    QString ratingType;
    QString visibility;
    bool passwordProtected;
    QString state;
    int replayDelaySeconds;
    int numPlayers;
    int maxPlayers;
    QMap<QString, QStringList> teams;
};

Q_DECLARE_METATYPE(TafLobbyGameInfo);
Q_DECLARE_METATYPE(QSharedPointer<TafLobbyGameInfo>);

class GameLaunchMsg
{
public:
    GameLaunchMsg();
    GameLaunchMsg(const QJsonObject& gameLaunchMsg);

    QString args;
    int initMode;
    QString mod;
    QString name;
    QString ratingType;
    int uid;
};

Q_DECLARE_METATYPE(GameLaunchMsg);
Q_DECLARE_METATYPE(QSharedPointer<GameLaunchMsg>);

class TafLobbyClient : public QObject
{
    Q_OBJECT

public:
    TafLobbyClient(QString userAgentName, QString userAgentVersion);
    ~TafLobbyClient();

    void connectToHost(QString hostName, quint16 port);
    void sendAskSession();
    void sendHello(qint64 session, QString uniqueId, QString localIp, QString login, QString password);
    void requestHostGame(QString title, QString password, QString mod, QString mapname, QString visibility, int replayDelaySeconds, QString ratingType);
    void sendPong();

signals:
    void notice(QString style, QString text);
    void session(qint64 sessionId);
    void welcome(QSharedPointer<TafLobbyPlayerInfo> playerInfo);
    void playerInfo(QSharedPointer<TafLobbyPlayerInfo> playerInfo);
    void gameInfo(QSharedPointer<TafLobbyGameInfo> gameInfo);
    void gameLaunch(QSharedPointer<GameLaunchMsg> gameLaunchMsg);

private:
    TafLobbyJsonProtocol m_protocol;

    void timerEvent(QTimerEvent* event);
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);
    void onReadyRead();

    const QString m_userAgentName;
    const QString m_userAgentVersion;

    QString m_hostName;
    quint16 m_port;

    QTcpSocket m_socket;
 
};
