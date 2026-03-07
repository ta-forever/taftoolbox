#pragma once

#include "GamesListModel.h"
#include "IceAdapterService.h"
#include "GameLauncherService.h"
#include "NewGameInfo.h"

#include <QtCore/qjsonarray.h>

class GameService: public QObject
{
    Q_OBJECT

public:

    static GameService* initialise(QObject* parent);
    static GameService* getInstance();
    GameService(QObject* parent);

    GamesListModel* getServerGamesModel();

    void hostGame(const NewGameInfo& newGameInfo);
    void joinGame(int gameId);
    void launchNow();
    void stopGame();
    bool isHostingActive() const { return m_consolePort > 0; }
    int  currentGameUid()  const { return m_currentGameUid; }

signals:
    void hostingActiveChanged(bool active);

public slots:
    void updateServerGames(QSharedPointer<TafLobbyGameInfo> gameInfo);
    void onGameLaunchMessageWhileHosting(QSharedPointer<GameLaunchMsg> gameLaunchMsg);
    void onGameLaunchMessageWhileJoining(QSharedPointer<GameLaunchMsg> gameLaunchMsg);

private slots:
    void onGpgGameMsg(QString cmd, QJsonArray args);
    void onPeerConnected(int remotePlayerId, bool connected);

private:
    void launchGame(QSharedPointer<GameLaunchMsg> gameLaunchMsg);

    static GameService* m_gameService;
    GamesListModel m_serverGamesModel;

    int  m_expectedPeerCount  = 0;
    int  m_connectedPeerCount = 0;
    int  m_consolePort        = 0;
    bool m_isHost             = false;
    int  m_currentGameUid     = 0;

    enum class OnGameLaunchMsg
    {
        Ignore,
        Host,
        Join
    };
    OnGameLaunchMsg m_onGameLaunchMsg;
};
