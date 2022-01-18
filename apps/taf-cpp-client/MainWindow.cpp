#include "MainWindow.h"

#include "taflib/Logger.h"
#include "games/GamesWidget.h"

#include <QtCore/qfile.h>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow())
{
    qInfo() << "[MainWindow::MainWindow]";
    m_ui->setupUi(this);

    QFile css(":/MainWindow.css");
    css.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = QString(css.readAll()).replace("%THEMEPATH%", ":/res/images");
    this->setStyleSheet(data);
    
    m_ui->mainTabs->setTabIcon(m_ui->mainTabs->indexOf(m_ui->whatNewTab), QIcon::fromTheme(":/res/images/client/whatNew.png"));
    m_ui->mainTabs->setTabIcon(m_ui->mainTabs->indexOf(m_ui->gamesTab), QIcon::fromTheme(":/res/images/client/games.png"));
    m_ui->mainTabs->setTabIcon(m_ui->mainTabs->indexOf(m_ui->mapsTab), QIcon::fromTheme(":/res/images/client/maps.png"));
    m_ui->mainTabs->setTabIcon(m_ui->mainTabs->indexOf(m_ui->ladderTab), QIcon::fromTheme(":/res/images/client/ladder.png"));
    m_ui->mainTabs->setTabIcon(m_ui->mainTabs->indexOf(m_ui->replaysTab), QIcon::fromTheme(":/res/images/client/replays.png"));
    m_ui->gamesTab->layout()->addWidget(m_gamesWidget = new GamesWidget(this));
}

MainWindow::~MainWindow()
{
}


