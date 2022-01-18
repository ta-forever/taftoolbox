#include "PlayerService.h"

#include "TafService.h"

#include "tafclient/TafLobbyClient.h"
#include "taflib/Logger.h"

PlayerService* PlayerService::m_instance = 0;

PlayerService* PlayerService::initialise(QObject *parent)
{
    return m_instance = new PlayerService(parent);
}

PlayerService* PlayerService::getInstance()
{
    return m_instance;
}

PlayerService::PlayerService(QObject *parent):
    QObject(parent)
{
    qInfo() << "[PlayerService::PlayerService]";
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::playerInfo, this, &PlayerService::_updateServerPlayers);
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::welcome, this, &PlayerService::_updateCurrentUser);
}

DtoTableModel<TafLobbyPlayerDto>* PlayerService::getPlayersModel()
{
    return &m_players;
}

const TafLobbyPlayerDto* PlayerService::getPlayerById(int id)
{
    return m_players.getDtoById(id);
}

const TafLobbyPlayerDto* PlayerService::getPlayerByLogin(QString login)
{
    for (int row = 0; row < m_players.rowCount(); ++row)
    {
        if (m_players.getDtoByRow(row)->login == login)
        {
            return m_players.getDtoByRow(row);
        }
    }
    return NULL;
}

const TafLobbyPlayerInfo* PlayerService::getCurrentUser()
{
    return &m_currentUser;
}

void PlayerService::_updateServerPlayers(QSharedPointer<TafLobbyPlayerInfo> playerInfo)
{
    m_players.update(*playerInfo);
}

void PlayerService::_updateCurrentUser(QSharedPointer<TafLobbyPlayerInfo> playerInfo)
{
    m_currentUser = *playerInfo;
}
