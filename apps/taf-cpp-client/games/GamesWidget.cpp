#include "GamesWidget.h"

#include "GameService.h"
#include "GameCardDelegate.h"
#include "player/PlayerService.h"

#include "taflib/Logger.h"

#include <QtWidgets/qmenu.h>

static bool isJoinable(const QSharedPointer<TafLobbyGameInfo>& game)
{
    return game && (game->state == "staging" || game->state == "open");
}

static bool isWatchable(const QSharedPointer<TafLobbyGameInfo>& game)
{
    return game && game->state == "live";
}

GamesWidget::GamesWidget(QWidget* parent) :
    QWidget(parent),
    m_ui(new Ui::GamesWidget()),
    m_gameDialog(new GameCreateDialog(parent))
{
    m_ui->setupUi(this);
    m_ui->gameList->setModel(GameService::getInstance()->getServerGamesModel());
    m_ui->gameList->setItemDelegate(new GameCardDelegate());
    m_ui->gameList->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(m_ui->gameList->selectionModel(), &QItemSelectionModel::currentChanged,
                     this, &GamesWidget::onGameSelectionChanged);

    QObject::connect(m_ui->gameList, &QListView::doubleClicked, [](const QModelIndex& index) {
        auto game = index.data().value<QSharedPointer<TafLobbyGameInfo>>();
        if (isJoinable(game))
            GameService::getInstance()->joinGame(static_cast<int>(game->id));
    });

    QObject::connect(m_ui->gameList, &QListView::customContextMenuRequested,
                     this, &GamesWidget::onGameListContextMenu);

    QObject::connect(GameService::getInstance(), &GameService::hostingActiveChanged,
                     this, &GamesWidget::onHostingActiveChanged);
}

GamesWidget::~GamesWidget()
{ }

void GamesWidget::on_createGame_clicked()
{
    qInfo() << "[GamesWidget::on_createGame_clicked]";
    m_gameDialog->show();
}

void GamesWidget::on_joinGame_clicked()
{
    QModelIndex index = m_ui->gameList->currentIndex();
    auto game = index.data().value<QSharedPointer<TafLobbyGameInfo>>();
    if (isJoinable(game))
        GameService::getInstance()->joinGame(static_cast<int>(game->id));
}

void GamesWidget::onGameListContextMenu(const QPoint& pos)
{
    QModelIndex index = m_ui->gameList->indexAt(pos);
    if (!index.isValid())
        return;

    auto game = index.data().value<QSharedPointer<TafLobbyGameInfo>>();
    if (!game)
        return;

    const TafLobbyPlayerInfo* me = PlayerService::getInstance()->getCurrentUser();
    bool isMyGame = me && (game->host == me->login);

    QMenu menu(this);
    if (isJoinable(game) && !isMyGame)
        menu.addAction("Join Game", [game]() {
            GameService::getInstance()->joinGame(static_cast<int>(game->id));
        });
    if (isWatchable(game))
        menu.addAction("Watch Game");   // placeholder — spectate not yet implemented

    if (!menu.isEmpty())
        menu.exec(m_ui->gameList->viewport()->mapToGlobal(pos));
}

void GamesWidget::on_launchGame_clicked()
{
    GameService::getInstance()->launchNow();
}

void GamesWidget::on_closeGame_clicked()
{
    GameService::getInstance()->stopGame();
}

void GamesWidget::enforceSelectionLock()
{
    // When in a game, find our game in the list and force it selected
    int uid = GameService::getInstance()->currentGameUid();
    auto* model = GameService::getInstance()->getServerGamesModel();
    for (int row = 0; row < model->rowCount(QModelIndex()); ++row)
    {
        auto game = model->data(model->index(row), Qt::DisplayRole)
                        .value<QSharedPointer<TafLobbyGameInfo>>();
        if (game && static_cast<int>(game->id) == uid)
        {
            m_lockingSelection = true;
            m_ui->gameList->selectionModel()->setCurrentIndex(
                model->index(row), QItemSelectionModel::ClearAndSelect);
            m_lockingSelection = false;
            return;
        }
    }
}

void GamesWidget::onGameSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    if (m_lockingSelection)
        return;

    // If we're in an active game, keep the selection locked to that game
    if (GameService::getInstance()->isHostingActive())
    {
        enforceSelectionLock();
        return;
    }

    auto game = current.data().value<QSharedPointer<TafLobbyGameInfo>>();
    if (!game)
    {
        m_ui->joinGame->setEnabled(false);
        return;
    }

    const TafLobbyPlayerInfo* me = PlayerService::getInstance()->getCurrentUser();
    bool isMyGame = me && (game->host == me->login);
    m_ui->joinGame->setEnabled(!isMyGame && isJoinable(game));
}

void GamesWidget::onHostingActiveChanged(bool active)
{
    m_ui->launchGame->setEnabled(active);
    m_ui->closeGame->setEnabled(active);
    if (active)
        enforceSelectionLock();     // snap selection to our game immediately
    else
        m_ui->joinGame->setEnabled(false);
}
