#pragma once

#include "tafclient/TafLobbyClient.h"

#include <QtWidgets/qstyleditemdelegate.h>

class GameCardDelegate: public QStyledItemDelegate
{
public:
    GameCardDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int ICON_RECT = 100;
    const int ICON_CLIP_TOP_LEFT = 3;
    const int ICON_CLIP_BOTTOM_RIGHT = -7;
    const int  ICON_SHADOW_OFFSET = 8;
    const QColor SHADOW_COLOR = "#202020";
    const int FRAME_THICKNESS = 1;
    const QColor FRAME_COLOR = "#303030";
    const int SELECTED_FRAME_THICKNESS = 3;
    const QColor SELECTED_FRAME_COLOR = "#FFD700";
    const int TEXT_OFFSET = 10;
    const int TEXT_RIGHT_MARGIN = 5;

    const int TEXT_WIDTH = 250;
    const int ICON_SIZE = 110;
    const int PADDING = 10;

    const char* UNKNOWN_MAP_FILE_PATH = ":/res/games/unknown_map.png";
    const int STATUS_ICON_SIZE = 24;

    QString m_textFormat;
    QMap<QString, QString> m_colours;

    void _drawClearOption(QPainter* painter, const QStyleOptionViewItem& option) const;
    void _drawIconShadow(QPainter* painter, const QStyleOptionViewItem& option) const;
    void _drawIconBackground(QPainter* painter, const QStyleOptionViewItem& option) const;
    void _drawIcon(QPainter* painter, const QStyleOptionViewItem& option, QIcon icon) const;
    void _drawFrame(QPainter* painter, const QStyleOptionViewItem& option) const;
    void _drawText(QPainter* painter, const QStyleOptionViewItem& option, QString text) const;
    void _drawStatusIcon(QPainter* painter, const QStyleOptionViewItem& option, const QString& state) const;
    void _drawSelectionFrame(QPainter* painter, const QStyleOptionViewItem& option) const;

    mutable QMap<QString, QSharedPointer<QIcon>> m_iconCache;
    mutable QMap<QString, QSharedPointer<QIcon>> m_statusIconCache;
    QIcon& _getIconCache(QString filename) const;
    QIcon& _getStatusIconCache(const QString& filename) const;
};
