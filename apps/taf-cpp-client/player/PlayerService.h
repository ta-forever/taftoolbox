#pragma once

#include "DtoTableModel.h"
#include "TafLobbyPlayerDto.h"

#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

class PlayerService: public QObject
{
    Q_OBJECT
public:
    static PlayerService* initialise(QObject *parent);
    static PlayerService* getInstance();

    DtoTableModel<TafLobbyPlayerDto>* getPlayersModel();
    const TafLobbyPlayerDto* getPlayerById(int id);
    const TafLobbyPlayerDto* getPlayerByLogin(QString login);
    const TafLobbyPlayerInfo* getCurrentUser();

private:
    static PlayerService* m_instance;
    DtoTableModel<TafLobbyPlayerDto> m_players;
    TafLobbyPlayerInfo m_currentUser;

    PlayerService(QObject *parent);
    void _updateServerPlayers(QSharedPointer<TafLobbyPlayerInfo> playerInfo);
    void _updateCurrentUser(QSharedPointer<TafLobbyPlayerInfo> playerInfo);
};
