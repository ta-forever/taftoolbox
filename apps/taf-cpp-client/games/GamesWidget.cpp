#include "GamesWidget.h"

#include "GameService.h"
#include "GameCardDelegate.h"

#include "taflib/Logger.h"

GamesWidget::GamesWidget(QWidget* parent) :
    QWidget(parent),
    m_ui(new Ui::GamesWidget()),
    m_gameDialog(new GameCreateDialog(parent))
{
    m_ui->setupUi(this);
    m_ui->gameList->setModel(GameService::getInstance()->getServerGamesModel());
    m_ui->gameList->setItemDelegate(new GameCardDelegate());
}

GamesWidget::~GamesWidget()
{ }

void GamesWidget::on_createGame_clicked()
{
    qInfo() << "[GamesWidget::on_createGame_clicked]";
    m_gameDialog->show();
}
