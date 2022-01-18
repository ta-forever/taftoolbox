#pragma once

#include <QtCore/qmap.h>
#include <QtCore/qabstractitemmodel.h>

template<typename T>
class DtoTableModel : public QAbstractTableModel
{
public:
    DtoTableModel(QObject* parent = NULL) :
        QAbstractTableModel(parent)
    { }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return m_items.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return int(typename T::Fields::_COLUMN_COUNT);
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.isValid() && index.row() >= 0 && index.row() < m_items.size() && Qt::DisplayRole == role)
        {
            return m_items.at(index.row()).get(T::Fields(index.column()));
        }
        return QVariant();
    }

    void append(const T& item)
    {
        int nextRow = m_items.size();
        beginInsertRows(QModelIndex(), nextRow, nextRow);
        m_items.append(item);
        m_itemsById.insert(item.id(), nextRow);
        endInsertRows();
    }

    void clear()
    {
        beginRemoveRows(QModelIndex(), 0, m_items.size() -1);
        m_items.clear();
        m_itemsById.clear();
        endRemoveRows();
    }

    template<typename IteratorT>
    void append(IteratorT begin, IteratorT end)
    {
        if (begin == end)
        {
            return;
        }
        int nextRow = m_items.size();
        int lastRow = nextRow + std::distance(begin, end) - 1;
        beginInsertRows(QModelIndex(), nextRow, lastRow);
        for (auto it = begin; it != end; ++it, ++nextRow)
        {
            m_items.append(*it);
            m_itemsById.insert(it->id(), nextRow);
        }
        endInsertRows();
    }

    void update(const T& item)
    {
        auto it = m_itemsById.find(item.id());
        if (it != m_itemsById.end())
        {
            int row = it.value();
            if (row < 0 || row >= m_items.size())
            {
                qWarning() << "[DtoTableModel::update] invalid row number for existing item!";
            }
            else
            {
                m_items[row] = item;
                QModelIndex indexFrom = this->index(row, 0);
                QModelIndex indexTo = this->index(row, int(typename T::Fields::_COLUMN_COUNT) -1);
                emit dataChanged(indexFrom, indexTo);
            }
        }
        else
        {
            append(item);
        }
    }

    void remove(typename T::IdType id)
    {
        int removedRow = m_itemsById.value(id, -1);
        if (removedRow >= 0 && removedRow < m_games.size())
        {
            beginRemoveRows(QModelIndex(), removedRow, removedRow);
            m_items.remove(removedRow);
            m_itemsById.clear();
            for (int n = 0; n < m_items.size(); ++n)
            {
                m_itemsById.insert(m_items[n].id(), n);
            }
            endRemoveRows();
        }
    }

    const T* getDtoById(typename T::IdType id) const
    {
        int row = m_itemsById.value(id, -1);
        if (row >= 0 && row < m_items.size())
        {
            return &m_items[row];
        }
        else
        {
            return NULL;
        }
    }

    int getRowById(typename T::IdType id) const
    {
        return m_itemsById.value(id, -1);
    }

    const T* getDtoByRow(int row) const
    {
        if (row >= 0 && row < m_items.size())
        {
            return &m_items[row];
        }
        else
        {
            return NULL;
        }
    }

private:
    QVector<T> m_items;
    QMap<typename T::IdType, int> m_itemsById;
};
