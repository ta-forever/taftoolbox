#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#include <QtGui/qfontdatabase.h>
#include <QtWidgets/qapplication.h>

#include "gpgnet/GpgNetParse.h"
#include "tafclient/TafLobbyClient.h"
#include "tafclient/TafHwIdGenerator.h"
#include "taflib/Logger.h"

#include "DownloadService.h"
#include "LoginDialog.h"
#include "NativeTools.h"
#include "MainWindow.h"
#include "TafService.h"
#include "games/GameService.h"
#include "games/IceAdapterService.h"
#include "games/GameLauncherService.h"
#include "maps/MapService.h"
#include "mods/ModService.h"
#include "player/PlayerService.h"
#include "preferences/PreferencesService.h"
#include "ta/MapTool.h"

#include <iostream>

int doMain(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("TA Forever");
    QApplication::setApplicationName("taf-cpp-client");
    QApplication::setApplicationVersion("0.14.11");

    // bundle our own font so rendering doesn't depend on Windows system fonts
    // (Segoe UI/Verdana are absent under Wine/CrossOver and its fallback is ugly)
    if (QFontDatabase::addApplicationFont(":/res/fonts/DejaVuSans.ttf") >= 0)
    {
        QFontDatabase::addApplicationFont(":/res/fonts/DejaVuSans-Bold.ttf");
        QApplication::setFont(QFont("DejaVu Sans", 9));
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("TA Forever c++ Client");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("logfile", "path to file in which to write logs.", "logfile", ""));
    parser.addOption(QCommandLineOption("loglevel", "level of noise in log files. 0 (silent) to 5 (debug).", "logfile", "5"));
    parser.addOption(QCommandLineOption(
        "serverConfigUrl", "url from which to retrieve server config data","serverConfigUrl", "https://content.taforever.com/dfc-config.json"));
    parser.addOption(QCommandLineOption(
        "native-dir",
        "Path to directory containing native binaries (maptool.exe, gpgnet4ta.exe, "
        "talauncher.exe, faf-uid.exe, faf-ice-adapter.jar). "
        "Overrides the saved preference and is persisted for future runs.",
        "path"));
    parser.process(app);

    QString logFile = parser.value("logfile");
    if (logFile.isEmpty())
    {
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
        QDir().mkpath(logDir);
        logFile = logDir + "/client.log";
    }
    taflib::Logger::Initialise(logFile.toStdString(), taflib::Logger::Verbosity(parser.value("loglevel").toInt()));
    qInstallMessageHandler(taflib::Logger::Log);

    // PreferencesService must be first — other services depend on it
    PreferencesService::initialise(&app);

    // --native-dir overrides (and persists) the saved nativeDir preference
    if (parser.isSet("native-dir"))
        PreferencesService::getInstance()->setNativeDir(parser.value("native-dir"));

    QString nativeDir = PreferencesService::getInstance()->getNativeDir();
    QString mapsCache = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/maps";
    MapTool::initialise(nativeDir + "/bin/" + NativeTools::exeName("maptool"), mapsCache);

    TafService::initialise(&app);
    GameService::initialise(&app);
    IceAdapterService::initialise(&app);
    GameLauncherService::initialise(&app);
    MapService::initialise(&app);
    ModService::initialise();
    PlayerService::initialise(&app);

    MainWindow mainWindow;
    LoginDialog *loginDialog = new LoginDialog(&mainWindow);

    QObject::connect(loginDialog, &LoginDialog::loginRequested, [loginDialog](QString username, QString password, const TafEndpoints& endpoints) {
        loginDialog->showStatus("Connecting...");
        TafService::getInstance()->setTafEndpoints(endpoints);
        TafService::getInstance()->getTafLobbyClient()->connectToHost(endpoints.lobbyHost, endpoints.lobbyPort);
    });
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::welcome, [&loginDialog, &mainWindow](QSharedPointer<TafLobbyPlayerInfo> playerInfo) {
        loginDialog->hide();
        mainWindow.show();
    });
    // login failures used to be log-only, leaving the dialog looking like
    // "nothing happened" — show them on the login banner instead
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::authenticationFailed, [loginDialog](QString text) {
        loginDialog->showStatus(text);
    });
    QObject::connect(TafService::getInstance()->getTafLobbyClient(), &TafLobbyClient::notice, [loginDialog, &mainWindow](QString style, QString text) {
        if (style == "error" && !mainWindow.isVisible())
        {
            loginDialog->showStatus(text);
        }
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
