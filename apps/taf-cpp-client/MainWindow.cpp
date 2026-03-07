#include "MainWindow.h"

#include "taflib/Logger.h"
#include "games/GamesWidget.h"
#include "games/GameService.h"
#include "games/GameLauncherService.h"
#include "games/IceAdapterService.h"
#include "TafService.h"

#include <QtCore/qfile.h>
#include <QtCore/qstandardpaths.h>

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

    // Connection indicator — small circle in the top bar, green=connected red=not
    m_connectionIndicator = new QPushButton(this);
    m_connectionIndicator->setFlat(true);
    m_connectionIndicator->setFixedSize(16, 16);
    m_connectionIndicator->setToolTip("Server connection — click to reconnect");
    m_connectionIndicator->setCursor(Qt::PointingHandCursor);
    m_ui->topLayout->addStretch();
    m_ui->topLayout->addWidget(m_connectionIndicator);
    onConnectionStateChanged(TafService::getInstance()->getTafLobbyClient()->isConnected());

    auto* lobby = TafService::getInstance()->getTafLobbyClient();
    QObject::connect(lobby, &TafLobbyClient::connectionStateChanged,
                     this, &MainWindow::onConnectionStateChanged);
    QObject::connect(m_connectionIndicator, &QPushButton::clicked,
                     lobby, &TafLobbyClient::disconnectFromHost);

    m_ui->actionShowLogs->setCheckable(true);
    QObject::connect(m_ui->actionShowLogs, &QAction::toggled, this, &MainWindow::onShowLogsToggled);
    QObject::connect(GameService::getInstance(), &GameService::hostingActiveChanged,
                     this, &MainWindow::onHostingActiveChanged);
}

MainWindow::~MainWindow()
{
}

void MainWindow::onShowLogsToggled(bool checked)
{
    if (checked)
    {
        if (!m_clientLogWindow)
        {
            QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                              + "/logs/client.log";
            m_clientLogWindow = new LogWindow(logPath, this);
        }
        m_clientLogWindow->show();
        m_clientLogWindow->raise();

        if (GameService::getInstance()->isHostingActive())
            openGameLogWindows();
    }
    else
    {
        if (m_clientLogWindow)
            m_clientLogWindow->hide();
        closeGameLogWindows();
    }
}

void MainWindow::onHostingActiveChanged(bool active)
{
    if (active)
    {
        if (m_ui->actionShowLogs->isChecked())
            openGameLogWindows();
    }
    else
    {
        closeGameLogWindows();
    }
}

void MainWindow::openGameLogWindows()
{
    auto* launcher = GameLauncherService::getInstance();
    auto* ice      = IceAdapterService::getInstance();

    auto open = [this](const QString& path) {
        if (path.isEmpty())
            return;
        auto* w = new LogWindow(path, this);
        m_gameLogWindows << w;
        w->show();
    };

    open(launcher->getGpgNetLogPath());
    open(launcher->getLaunchServerLogPath());
    open(ice->getLogPath());
}

void MainWindow::closeGameLogWindows()
{
    for (auto* w : m_gameLogWindows)
        w->deleteLater();
    m_gameLogWindows.clear();
}

void MainWindow::onConnectionStateChanged(bool connected)
{
    if (connected)
        m_connectionIndicator->setStyleSheet(
            "QPushButton { border-radius: 7px; background-color: #00cc00;"
            " border: 1px solid #009900; }");
    else
        m_connectionIndicator->setStyleSheet(
            "QPushButton { border-radius: 7px; background-color: #cc0000;"
            " border: 1px solid #990000; }");
}
