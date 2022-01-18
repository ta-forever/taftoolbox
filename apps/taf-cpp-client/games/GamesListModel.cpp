#include "GamesListModel.h"

GamesListModel::GamesListModel()
{ }

int GamesListModel::rowCount(const QModelIndex& parent) const
{
    return m_games.size();
}

QVariant GamesListModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.row() >= 0 && index.row() < m_games.size() && Qt::DisplayRole == role)
    {
        const QSharedPointer<TafLobbyGameInfo> gameInfo = m_games[index.row()];
        return QVariant::fromValue(gameInfo);
    }
    return QVariant();
}

// required for QAbstractItemModel
//int GamesListModel::columnCount(const QModelIndex& parent) const
//{
//    return 10;
//}
//
//QModelIndex GamesListModel::index(int row, int column, const QModelIndex& parent) const
//{
//    return createIndex(row, column, nullptr);
//}
//
//QModelIndex GamesListModel::parent(const QModelIndex& child) const
//{
//    return QModelIndex();
//}


const QSharedPointer<TafLobbyGameInfo> GamesListModel::getGame(int id) const
{
    int gameNumber = m_gameNumberById.value(id, -1);
    if (gameNumber >= 0 && gameNumber < m_games.size())
    {
        return m_games[gameNumber];
    }
    else
    {
        return QSharedPointer<TafLobbyGameInfo>();
    }
}

void GamesListModel::updateGame(QSharedPointer<TafLobbyGameInfo> gameInfo)
{
    auto it = m_gameNumberById.find(gameInfo->id);
    if (it != m_gameNumberById.end())
    {
        int row = it.value();
        if (row < 0 || row >= m_games.size())
        {
            qWarning() << "[GamesListModel::updateGame] invalid row number for existing game!";
        }
        else if (!gameInfo->state.compare("ENDED", Qt::CaseInsensitive))
        {
            _removeGame(gameInfo);
        }
        else
        {
            m_games[row] = gameInfo;
            QModelIndex index = this->index(row);
            emit dataChanged(index, index);
        }
    }
    else
    {
        _appendGame(gameInfo);
    }
}

void GamesListModel::_appendGame(QSharedPointer<TafLobbyGameInfo> gameInfo)
{
    int nextRow = m_games.size();
    beginInsertRows(QModelIndex(), nextRow, nextRow);
    m_games.append(gameInfo);
    m_gameNumberById.insert(gameInfo->id, nextRow);
    endInsertRows();
}

void GamesListModel::_removeGame(QSharedPointer<TafLobbyGameInfo> gameInfo)
{
    int removedRow = m_gameNumberById.value(gameInfo->id, -1);
    if (removedRow >= 0 && removedRow < m_games.size())
    {
        beginRemoveRows(QModelIndex(), removedRow, removedRow);
        m_games.remove(removedRow);
        m_gameNumberById.clear();
        for (int n = 0; n < m_games.size(); ++n)
        {
            m_gameNumberById.insert(m_games[n]->id, n);
        }
        endRemoveRows();
    }
}
