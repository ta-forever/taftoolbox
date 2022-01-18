#pragma once

#include "tafclient/TafLobbyClient.h"

#include <QtCore/qmap.h>
#include <QtCore/qabstractitemmodel.h>

class GamesListModel : public QAbstractListModel
{
public:
    GamesListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // required for QAbstractItemModel
    //int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    //QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override;
    //QModelIndex parent(const QModelIndex& child) const override;

    void updateGame(QSharedPointer<TafLobbyGameInfo> gameInfo);
    const QSharedPointer<TafLobbyGameInfo> getGame(int id) const;

private:
    QVector<QSharedPointer<TafLobbyGameInfo> > m_games;
    QMap<int, int> m_gameNumberById;

    void _appendGame(QSharedPointer<TafLobbyGameInfo> gameInfo);
    void _removeGame(QSharedPointer<TafLobbyGameInfo> gameInfo);
};