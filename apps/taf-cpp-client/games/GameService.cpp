#include "GameService.h"
#include "TafService.h"
#include "maps/MapService.h"
#include "mods/ModService.h"
#include "player/PlayerService.h"
#include "preferences/PreferencesService.h"

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

    // Joiner: auto-launch when the host transitions the game out of staging
    if (!m_isHost && m_consolePort > 0
        && m_currentGameUid > 0 && gameInfo->id == m_currentGameUid
        && gameInfo->state != "staging" && gameInfo->state != "open"
        && gameInfo->state.compare("ENDED", Qt::CaseInsensitive) != 0)
    {
        qInfo() << "[GameService::updateServerGames] game" << gameInfo->id
                << "transitioned to" << gameInfo->state << "- launching as joiner";
        m_currentGameUid = 0;   // prevent re-triggering
        launchNow();
    }

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

void GameService::joinGame(int gameId)
{
    qInfo() << "[GameService::joinGame]" << gameId;
    QObject::disconnect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::gameLaunch, this, &GameService::onGameLaunchMessageWhileHosting);
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::gameLaunch, this, &GameService::onGameLaunchMessageWhileJoining);
    TafService::getInstance()->getTafLobbyClient()->requestJoinGame(gameId, "");
}

void GameService::launchGame(QSharedPointer<GameLaunchMsg> msg)
{
    qInfo() << "[GameService::launchGame] uid" << msg->uid << "mod" << msg->mod;

    QString modPath = ModService::getInstance()->getModPath(msg->mod);
    if (modPath.isEmpty())
    {
        qWarning() << "[GameService::launchGame] no mod path for" << msg->mod << "- aborting";
        return;
    }

    const TafLobbyPlayerInfo* me = PlayerService::getInstance()->getCurrentUser();
    int     playerId    = me ? static_cast<int>(me->id) : 0;
    QString playerLogin = me ? me->login : QString("unknown");

    int launchServerPort = GameLauncherService::findFreePort();
    m_consolePort        = GameLauncherService::findFreePort();
    int iceRpcPort       = IceAdapterService::findFreePort();
    int iceGpgNetPort    = IceAdapterService::findFreePort();

    m_expectedPeerCount  = 0;
    m_connectedPeerCount = 0;
    emit hostingActiveChanged(true);

    GameLauncherService::getInstance()->registerDplay(msg->mod, modPath, msg->uid);
    GameLauncherService::getInstance()->startLaunchServer(launchServerPort, msg->uid);

    auto* lobby    = TafService::getInstance()->getTafLobbyClient();
    auto* ice      = IceAdapterService::getInstance();
    auto* launcher = GameLauncherService::getInstance();

    ice->start(playerId, msg->uid, playerLogin, iceRpcPort, iceGpgNetPort);

    QObject::connect(ice, &IceAdapterService::rpcConnected, this,
        [this, msg, modPath, iceGpgNetPort, launchServerPort, launcher, ice]()
        {
            ice->setLobbyInitMode("normal");
            launcher->startGpgNet4ta(
                msg->uid, msg->mod, modPath,
                QString("127.0.0.1:%1").arg(iceGpgNetPort),
                launchServerPort, m_consolePort);
            launcher->startKeepalive(m_consolePort);
        },
        Qt::UniqueConnection);

    QObject::connect(lobby, &TafLobbyClient::iceServersReceived,
                     ice,   &IceAdapterService::setIceServers,
                     Qt::UniqueConnection);

    QObject::connect(lobby, &TafLobbyClient::gpgGameMsg,
                     this,  &GameService::onGpgGameMsg,
                     Qt::UniqueConnection);

    QObject::connect(ice,  &IceAdapterService::onIceMsg,
                     lobby, &TafLobbyClient::sendIceMessage,
                     Qt::UniqueConnection);

    QObject::connect(ice,  &IceAdapterService::onConnected,
                     this,  &GameService::onPeerConnected,
                     Qt::UniqueConnection);

    QObject::connect(ice,  &IceAdapterService::gpgNetMessageReceived,
                     lobby, &TafLobbyClient::sendGpgGameMsg,
                     Qt::UniqueConnection);

    QObject::connect(ice,  &IceAdapterService::gameConnectionLost,
                     this, &GameService::stopGame,
                     Qt::UniqueConnection);
}

void GameService::launchNow()
{
    if (m_consolePort > 0)
    {
        qInfo() << "[GameService::launchNow] sending /launch";
        GameLauncherService::getInstance()->sendConsoleCommand("/launch", m_consolePort);
    }
}

void GameService::stopGame()
{
    qInfo() << "[GameService::stopGame]";
    if (m_consolePort > 0)
    {
        // Notify the lobby server so it removes the game from the board
        TafService::getInstance()->getTafLobbyClient()->sendGpgGameMsg("GameState", QJsonArray{"Ended"});
    }
    IceAdapterService::getInstance()->stop();
    GameLauncherService::getInstance()->stopAll();
    m_consolePort        = 0;
    m_expectedPeerCount  = 0;
    m_connectedPeerCount = 0;
    m_isHost             = false;
    m_currentGameUid     = 0;
    emit hostingActiveChanged(false);
}

void GameService::onGameLaunchMessageWhileHosting(QSharedPointer<GameLaunchMsg> gameLaunchMsg)
{
    qInfo() << "[GameService::onGameLaunchMessageWhileHosting]";
    m_isHost = true;
    m_currentGameUid = gameLaunchMsg->uid;
    launchGame(gameLaunchMsg);
}

void GameService::onGameLaunchMessageWhileJoining(QSharedPointer<GameLaunchMsg> gameLaunchMsg)
{
    qInfo() << "[GameService::onGameLaunchMessageWhileJoining]";
    m_isHost = false;
    m_currentGameUid = gameLaunchMsg->uid;
    launchGame(gameLaunchMsg);
}

void GameService::onGpgGameMsg(QString cmd, QJsonArray args)
{
    qInfo() << "[GameService::onGpgGameMsg]" << cmd;
    auto* ice = IceAdapterService::getInstance();
    if (cmd == "HostGame")
    {
        ice->hostGame(args[0].toString());
    }
    else if (cmd == "JoinGame")
    {
        ice->joinGame(args[0].toString(), args[1].toInt());
        m_expectedPeerCount++;
    }
    else if (cmd == "ConnectToPeer")
    {
        bool offer = args.size() > 2 && args[2].toBool();
        ice->connectToPeer(args[0].toString(), args[1].toInt(), offer);
        m_expectedPeerCount++;
    }
    else if (cmd == "DisconnectFromPeer")
    {
        ice->disconnectFromPeer(args[0].toInt());
    }
    else if (cmd == "IceMsg")
    {
        ice->iceMsg(args[0].toInt(), args[1]);
    }
    else if (cmd == "GameState" && !args.isEmpty() && args[0].toString() == "Ended")
    {
        stopGame();
    }
}

void GameService::onPeerConnected(int remotePlayerId, bool connected)
{
    qInfo() << "[GameService::onPeerConnected]" << remotePlayerId << connected;
    if (connected)
        m_connectedPeerCount++;
    if (m_isHost && m_expectedPeerCount > 0 && m_connectedPeerCount >= m_expectedPeerCount)
    {
        qInfo() << "[GameService::onPeerConnected] all peers connected, sending /launch";
        GameLauncherService::getInstance()->sendConsoleCommand("/launch", m_consolePort);
    }
}
