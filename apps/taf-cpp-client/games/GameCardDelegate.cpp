#include "GameCardDelegate.h"
#include "taflib/Logger.h"
#include "maps/MapService.h"

#include <QtCore/qfile.h>
#include <QtGui/qpainter.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtGui/qtextdocument.h>


GameCardDelegate::GameCardDelegate()
{
    {
        QFile fp(":/res/games/formatters/game_card_text.qthtml");
        fp.open(QIODevice::ReadOnly | QIODevice::Text);
        m_textFormat = fp.readAll();
    }

    {
        QFile fp(":/res/client/colours.json");
        fp.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray json = fp.readAll();
        QJsonDocument jdoc;
        jdoc.fromJson(json);
        for (auto it = jdoc.object().begin(); it != jdoc.object().end(); ++it)
        {
            m_colours[it.key()] = it.value().toString();
        }
    }
}

void GameCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto gameInfo = index.data().value<QSharedPointer<TafLobbyGameInfo>>();

    QString text = QString(m_textFormat)
        .replace("{color}", m_colours.value("player", "silver"))
        .replace("{mapslots}", QString::number(gameInfo->maxPlayers))
        .replace("{mapdisplayname}", gameInfo->mapName.toHtmlEscaped())
        .replace("{title}", gameInfo->title.toHtmlEscaped())
        .replace("{host}", gameInfo->host.toHtmlEscaped())
        .replace("{players}", QString::number(gameInfo->numPlayers))
        .replace("{playerstring}", gameInfo->numPlayers == 1 ? "player" : "players")
        .replace("{avgrating}", gameInfo->ratingType.toHtmlEscaped());

    QString previewFilename = MapService::getInstance()->getPreviewCacheFilePath(gameInfo->mapName, MapPreviewType::Mini, 10);

    painter->save();
    _drawClearOption(painter, option);
    _drawIconShadow(painter, option);
    _drawIconBackground(painter, option);
    _drawIcon(painter, option, _getIconCache(previewFilename));
    _drawFrame(painter, option);
    _drawText(painter, option, text);
    painter->restore();
}

QSize GameCardDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(ICON_SIZE + TEXT_WIDTH + PADDING, ICON_SIZE);
}

void GameCardDelegate::_drawClearOption(QPainter* painter, const QStyleOptionViewItem& option) const
{
    option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);
}

void GameCardDelegate::_drawIconShadow(QPainter* painter, const QStyleOptionViewItem& option) const
{
    painter->fillRect(
        option.rect.left() + ICON_SHADOW_OFFSET,
        option.rect.top() + ICON_SHADOW_OFFSET,
        ICON_RECT,
        ICON_RECT,
        SHADOW_COLOR
    );
}

void GameCardDelegate::_drawIconBackground(QPainter* painter, const QStyleOptionViewItem& option) const
{
    QPen pen;
    pen.setBrush(QColor("black"));
    painter->setPen(pen);
    painter->fillRect(
        option.rect.left() + ICON_CLIP_TOP_LEFT,
        option.rect.top() + ICON_CLIP_TOP_LEFT,
        ICON_RECT,
        ICON_RECT,
        Qt::BrushStyle::SolidPattern);
}

void GameCardDelegate::_drawIcon(QPainter* painter, const QStyleOptionViewItem& option, QIcon icon) const
{
    QRect rect = option.rect.adjusted(
        ICON_CLIP_TOP_LEFT,
        ICON_CLIP_TOP_LEFT,
        ICON_CLIP_BOTTOM_RIGHT,
        ICON_CLIP_BOTTOM_RIGHT);
    rect.setWidth(ICON_RECT);
    rect.setHeight(ICON_RECT);

    icon.paint(painter, rect, Qt::AlignCenter);
}

void GameCardDelegate::_drawFrame(QPainter* painter, const QStyleOptionViewItem& option) const
{
    QPen pen;
    pen.setWidth(FRAME_THICKNESS);
    pen.setBrush(FRAME_COLOR);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);
    painter->drawRect(
        option.rect.left() + ICON_CLIP_TOP_LEFT,
        option.rect.top() + ICON_CLIP_TOP_LEFT,
        ICON_RECT,
        ICON_RECT);
}

void GameCardDelegate::_drawText(QPainter* painter, const QStyleOptionViewItem& option, QString text) const
{
    int leftOffset = ICON_RECT + TEXT_OFFSET;
    int topOffset = TEXT_OFFSET;
    int rightOffset = TEXT_RIGHT_MARGIN;
    int bottomOffset = 0;
    painter->translate(
        option.rect.left() + leftOffset,
        option.rect.top() + topOffset);
    QRectF clip = QRectF(
        0, 0,
        option.rect.width() - leftOffset - rightOffset,
        option.rect.height() - topOffset - bottomOffset);
    QTextDocument html;
    html.setHtml(text);
    html.drawContents(painter, clip);
}

QIcon& GameCardDelegate::_getIconCache(QString filename) const
{
    if (!QFile(filename).exists())
    {
        filename = UNKNOWN_MAP_FILE_PATH;
    }

    QSharedPointer<QIcon> & icon = m_iconCache[filename];
    if (icon.isNull())
    {
        QPixmap pixmap(filename);
        pixmap = pixmap.scaled(ICON_RECT, ICON_RECT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        icon.reset(new QIcon(pixmap));
    }
    return *icon;
}
