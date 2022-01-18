#include "GameService.h"
#include "TafService.h"
#include "maps/MapService.h"

#include <QtGui/qpixmap.h>
#include <QtCore/qfile.h>

GameService* GameService::m_gameService = NULL;

GameService* GameService::initialise(QObject* parent)
{
    return m_gameService = new GameService(parent);
}

GameService* GameService::getInstance()
{
    return m_gameService;
}

GameService::GameService(QObject* parent) :
    QObject(parent),
    m_onGameLaunchMsg(OnGameLaunchMsg::Ignore)
{
    qInfo() << "[GameService::GameService]";
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::gameInfo, this, &GameService::updateServerGames);
}

GamesListModel* GameService::getServerGamesModel()
{
    return &m_serverGamesModel;
}

void GameService::updateServerGames(QSharedPointer<TafLobbyGameInfo> gameInfo)
{
    qInfo() << "[GameService::updateServerGames]" << gameInfo->id << gameInfo->title << gameInfo->host << gameInfo->state;
    m_serverGamesModel.updateGame(gameInfo);

    if (!MapService::getInstance()->isPreviewAvailable(gameInfo->mapName, MapPreviewType::Mini, 10))
    {
        MapService::getInstance()->getPreview(gameInfo->mapName, MapPreviewType::Mini, 10, gameInfo->featuredMod, [this, gameInfo](QString filePath) {
            if (MapService::getInstance()->isPreviewAvailable(gameInfo->mapName, MapPreviewType::Mini, 10))
            {
                this->updateServerGames(gameInfo);
            }
        });
    }
}

void GameService::hostGame(const NewGameInfo& newGameInfo)
{
    QObject::disconnect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::gameLaunch, this, &GameService::onGameLaunchMessageWhileJoining);
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::gameLaunch, this, &GameService::onGameLaunchMessageWhileHosting);
    TafService::getInstance()->getTafLobbyClient()->requestHostGame(
        newGameInfo.title,
        newGameInfo.password,
        newGameInfo.mod,
        newGameInfo.mapname,
        newGameInfo.visibility,
        newGameInfo.replayDelaySeconds,
        newGameInfo.ratingType);
}

void GameService::onGameLaunchMessageWhileHosting(QSharedPointer<GameLaunchMsg> gameLaunchMsg)
{
    qInfo() << "[GameService::onGameLaunchMessageWhileHosting]";
}

void GameService::onGameLaunchMessageWhileJoining(QSharedPointer<GameLaunchMsg> gameLaunchMsg)
{
    qInfo() << "[GameService::onGameLaunchMessageWhileJoining]";
}
