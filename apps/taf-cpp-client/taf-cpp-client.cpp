#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcryptographichash.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtWidgets/qapplication.h>

#include "gpgnet/GpgNetParse.h"
#include "tafclient/TafLobbyClient.h"
#include "tafclient/TafHwIdGenerator.h"
#include "taflib/Logger.h"

#include "DownloadService.h"
#include "LoginDialog.h"
#include "MainWindow.h"
#include "TafService.h"
#include "games/GameService.h"
#include "maps/MapService.h"
#include "mods/ModService.h"
#include "player/PlayerService.h"
#include "ta/MapTool.h"

#include <iostream>

int doMain(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("TA Forever");
    QApplication::setApplicationName("taf-cpp-client");
    QApplication::setApplicationVersion("0.14.11");

    QCommandLineParser parser;
    parser.setApplicationDescription("TA Forever c++ Client");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("logfile", "path to file in which to write logs.", "logfile", ""));
    parser.addOption(QCommandLineOption("loglevel", "level of noise in log files. 0 (silent) to 5 (debug).", "logfile", "5"));
    parser.addOption(QCommandLineOption(
        "serverConfigUrl", "url from which to retrieve server config data","serverConfigUrl", "https://content.taforever.com/dfc-config.json"));
    parser.process(app);

    taflib::Logger::Initialise(parser.value("logfile").toStdString(), taflib::Logger::Verbosity(parser.value("loglevel").toInt()));
    qInstallMessageHandler(taflib::Logger::Log);

    TafService::initialise(&app);
    GameService::initialise(&app);
    MapService::initialise(&app);
    MapTool::initialise("C:/Program Files/TA Forever Client/natives/bin/maptool.exe", "C:/ProgramData/TAForever/cache/maps");
    ModService::initialise();
    PlayerService::initialise(&app);

    MainWindow mainWindow;
    LoginDialog *loginDialog = new LoginDialog(&mainWindow);

    QObject::connect(loginDialog, &LoginDialog::loginRequested, [](QString username, QString password, const TafEndpoints& endpoints) {
        TafService::getInstance()->setTafEndpoints(endpoints);
        TafService::getInstance()->getTafLobbyClient()->connectToHost(endpoints.lobbyHost, endpoints.lobbyPort);
    });
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::welcome, [&loginDialog, &mainWindow](QSharedPointer<TafLobbyPlayerInfo> playerInfo) {
        loginDialog->hide();
        mainWindow.show();
    });

    TafService::getInstance()->getDfcConfig(parser.value("serverConfigUrl"), [loginDialog](QSharedPointer<DtoTableModel<TafEndpoints> > endpointsTableModel) {
        loginDialog->setTafEndpoints(endpointsTableModel);
    });
    
    loginDialog->show();
    app.exec();
    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return doMain(argc, argv);
    }
    catch (std::exception & e)
    {
        std::cerr << "[main catch std::exception] " << e.what() << std::endl;
        qWarning() << "[main catch std::exception]" << e.what();
        return 1;
    }
    catch (...)
    {
        std::cerr << "[main catch ...] " << std::endl;
        qWarning() << "[main catch ...]";
        return 1;
    }
}
