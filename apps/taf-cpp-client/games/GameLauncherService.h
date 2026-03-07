#pragma once

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstring.h>

class GameLauncherService : public QObject
{
    Q_OBJECT

public:
    static GameLauncherService* initialise(QObject* parent);
    static GameLauncherService* getInstance();
    static int findFreePort();

    // Run talauncher --registerdplay synchronously (blocks up to 10 s)
    void registerDplay(QString mod, QString modPath, int gameUid);

    // Run talauncher --bindport in background
    void startLaunchServer(int port, int gameUid);

    // Run gpgnet4ta in background
    void startGpgNet4ta(int gameUid, QString mod, QString modPath,
                        QString gpgNetUrl, int launchServerPort, int consolePort);

    // Open a fresh TCP connection, send cmd, close
    void sendConsoleCommand(QString cmd, int consolePort);

    // Start sending /keepalive every 1 s to consolePort
    void startKeepalive(int consolePort);

    // Terminate all child processes and stop keepalive
    void stopAll();

    QString getGpgNetLogPath()       const { return m_gpgNet4taLogPath; }
    QString getLaunchServerLogPath() const { return m_launchServerLogPath; }

private:
    explicit GameLauncherService(QObject* parent);
    QString nativeDir() const;
    QString logDir() const;

    static GameLauncherService* m_instance;

    QProcess* m_launchServer     = nullptr;
    QProcess* m_gpgNet4ta        = nullptr;
    QTimer    m_keepaliveTimer;
    int       m_consolePort      = 0;
    int       m_launchServerPort = 0;
    QString   m_gpgNet4taLogPath;
    QString   m_launchServerLogPath;
};
