#pragma once

#include "GamesListModel.h"
#include "NewGameInfo.h"

class GameService: public QObject
{
    Q_OBJECT

public:

    static GameService* initialise(QObject* parent);
    static GameService* getInstance();
    GameService(QObject* parent);

    GamesListModel* getServerGamesModel();

    void hostGame(const NewGameInfo& newGameInfo);

public slots:
    void updateServerGames(QSharedPointer<TafLobbyGameInfo> gameInfo);
    void onGameLaunchMessageWhileHosting(QSharedPointer<GameLaunchMsg> gameLaunchMsg);
    void onGameLaunchMessageWhileJoining(QSharedPointer<GameLaunchMsg> gameLaunchMsg);

private:
    static GameService* m_gameService;
    GamesListModel m_serverGamesModel;

    enum class OnGameLaunchMsg
    {
        Ignore,
        Host,
        Join
    };
    OnGameLaunchMsg m_onGameLaunchMsg;
};


