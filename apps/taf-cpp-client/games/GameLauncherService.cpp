#include "GameLauncherService.h"

#include "NativeTools.h"
#include "preferences/PreferencesService.h"
#include "taflib/Logger.h"

#include <QtCore/qdir.h>
#include <QtCore/qstandardpaths.h>
#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>

GameLauncherService* GameLauncherService::m_instance = nullptr;

GameLauncherService* GameLauncherService::initialise(QObject* parent)
{
    return m_instance = new GameLauncherService(parent);
}

GameLauncherService* GameLauncherService::getInstance()
{
    return m_instance;
}

GameLauncherService::GameLauncherService(QObject* parent) :
    QObject(parent)
{
    QObject::connect(&m_keepaliveTimer, &QTimer::timeout, [this]() {
        if (m_consolePort > 0)
            sendConsoleCommand("/keepalive", m_consolePort);
        if (m_launchServerPort > 0)
            sendConsoleCommand("/keepalive", m_launchServerPort);
    });
}

QString GameLauncherService::nativeDir() const
{
    return PreferencesService::getInstance()->getNativeDir();
}

QString GameLauncherService::logDir() const
{
    return PreferencesService::getInstance()->getLogDir();
}

int GameLauncherService::findFreePort()
{
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, 0);
    int port = server.serverPort();
    server.close();
    return port;
}

void GameLauncherService::registerDplay(QString mod, QString modPath, int gameUid)
{
    QDir().mkpath(logDir());
    QString talauncherExe = nativeDir() + "/bin/talauncher.exe";
    QString logFile = logDir() + QString("/registerdplay-%1.log").arg(gameUid);

    QStringList args;
    args << "--registerdplay"
         << "--gamemod"  << mod
         << "--gamepath" << modPath
         << "--gameexe"  << "TotalA.exe"
         << "--logfile"  << logFile;

    QString program = NativeTools::wineWrap(talauncherExe, args);
    qInfo() << "[GameLauncherService::registerDplay]" << program << args;

    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForFinished(10000))
    {
        qWarning() << "[GameLauncherService::registerDplay] timed out";
        proc.kill();
    }
}

void GameLauncherService::startLaunchServer(int port, int gameUid)
{
    QDir().mkpath(logDir());
    QString talauncherExe = nativeDir() + "/bin/talauncher.exe";
    QString logFile = logDir() + QString("/talauncher-%1.log").arg(gameUid);

    QStringList args;
    args << "--bindport" << QString::number(port)
         << "--logfile"  << logFile;

    QString program = NativeTools::wineWrap(talauncherExe, args);
    qInfo() << "[GameLauncherService::startLaunchServer]" << program << args;

    if (m_launchServer)
    {
        m_launchServer->terminate();
        m_launchServer->waitForFinished(3000);
        delete m_launchServer;
    }
    m_launchServerPort = port;
    m_launchServerLogPath = logFile;
    m_launchServer = new QProcess(this);
    m_launchServer->start(program, args);

    // keepalive talauncher from t=0: it kills itself after 10s without one, and
    // gpgnet4ta doesn't connect to it until the ice adapter's JVM has booted —
    // which can take well over 10s on a cold start (esp. under Wine/CrossOver)
    m_keepaliveTimer.setInterval(1000);
    m_keepaliveTimer.start();
}

void GameLauncherService::startGpgNet4ta(int gameUid, QString mod, QString modPath,
                                          QString gpgNetUrl, int launchServerPort, int consolePort)
{
    QDir().mkpath(logDir());
    QString gpgnet4taExe = nativeDir() + "/bin/" + NativeTools::exeName("gpgnet4ta");
    QString logFile = logDir() + QString("/gpgnet4ta-%1.log").arg(gameUid);

    QStringList args;
    args << "--gpgnet"           << gpgNetUrl
         << "--launchserverport" << QString::number(launchServerPort)
         << "--gameid"           << QString::number(gameUid)
         << "--gamemod"          << mod
         << "--gamepath"         << modPath
         << "--consoleport"      << QString::number(consolePort)
         << "--logfile"          << logFile;

    qInfo() << "[GameLauncherService::startGpgNet4ta]" << gpgnet4taExe << args;

    if (m_gpgNet4ta)
    {
        m_gpgNet4ta->terminate();
        m_gpgNet4ta->waitForFinished(3000);
        delete m_gpgNet4ta;
    }
    m_gpgNet4taLogPath = logFile;
    m_gpgNet4ta = new QProcess(this);
    m_gpgNet4ta->start(gpgnet4taExe, args);
}

void GameLauncherService::sendConsoleCommand(QString cmd, int consolePort)
{
    QTcpSocket sock;
    sock.connectToHost("127.0.0.1", consolePort);
    if (sock.waitForConnected(500))
    {
        sock.write(cmd.toUtf8());
        sock.flush();
        sock.waitForBytesWritten(500);
    }
    sock.disconnectFromHost();
}

void GameLauncherService::startKeepalive(int consolePort)
{
    m_consolePort = consolePort;
    m_keepaliveTimer.setInterval(1000);
    m_keepaliveTimer.start();
}

void GameLauncherService::stopAll()
{
    m_keepaliveTimer.stop();
    m_launchServerPort = 0;
    m_consolePort = 0;
    m_gpgNet4taLogPath.clear();
    m_launchServerLogPath.clear();

    if (m_gpgNet4ta)
    {
        m_gpgNet4ta->terminate();
        m_gpgNet4ta->waitForFinished(3000);
        delete m_gpgNet4ta;
        m_gpgNet4ta = nullptr;
    }
    if (m_launchServer)
    {
        m_launchServer->terminate();
        m_launchServer->waitForFinished(3000);
        delete m_launchServer;
        m_launchServer = nullptr;
    }
}
