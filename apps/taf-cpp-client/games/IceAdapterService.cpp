#include "IceAdapterService.h"

#include "NativeTools.h"
#include "preferences/PreferencesService.h"
#include "taflib/Logger.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstandardpaths.h>
#include <QtNetwork/qtcpserver.h>

IceAdapterService* IceAdapterService::m_instance = nullptr;

IceAdapterService* IceAdapterService::initialise(QObject* parent)
{
    return m_instance = new IceAdapterService(parent);
}

IceAdapterService* IceAdapterService::getInstance()
{
    return m_instance;
}

IceAdapterService::IceAdapterService(QObject* parent) :
    QObject(parent)
{
    m_connectTimer.setInterval(500);
    QObject::connect(&m_connectTimer, &QTimer::timeout, this, &IceAdapterService::tryConnect);
    QObject::connect(&m_socket, &QTcpSocket::readyRead, this, &IceAdapterService::onSocketReadyRead);
    QObject::connect(&m_socket, &QTcpSocket::connected, this, &IceAdapterService::onSocketConnected);
}

int IceAdapterService::findFreePort()
{
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, 0);
    int port = server.serverPort();
    server.close();
    return port;
}

void IceAdapterService::start(int playerId, int gameUid, QString playerName, int rpcPort, int gpgNetPort)
{
    stop();
    m_rpcPort = rpcPort;
    m_rpcIdCounter = 0;
    m_readBuf.clear();

    QString nativeDir = PreferencesService::getInstance()->getNativeDir();
    QString jarPath = nativeDir + "/faf-ice-adapter.jar";

    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    m_iceAdapterLogPath = logDir + QString("/ice-adapter-%1.log").arg(gameUid);

    QStringList args;
    args << QString("-Dlogging.file.name=") + m_iceAdapterLogPath
         << "-jar" << jarPath
         << "--id"         << QString::number(playerId)
         << "--game-id"    << QString::number(gameUid)
         << "--login"      << playerName
         << "--rpc-port"   << QString::number(rpcPort)
         << "--gpgnet-port" << QString::number(gpgNetPort);

    // prefer a bundled JRE (required under Wine, where no Windows java.exe is
    // on PATH): first <nativeDir>/jre, then the taf-java-client install layout
    // where natives/ and jre/ are siblings; fall back to PATH otherwise
    QString javaExe = "java";
    for (QString candidate : { nativeDir + "/jre/bin/" + NativeTools::exeName("java"),
                               nativeDir + "/../jre/bin/" + NativeTools::exeName("java") })
    {
        if (QFileInfo(candidate).isFile())
        {
            javaExe = QDir::cleanPath(candidate);
            break;
        }
    }

    qInfo() << "[IceAdapterService::start]" << javaExe << args;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(m_process, &QProcess::readyReadStandardOutput, [this]() {
        qDebug() << "[ice-adapter]" << m_process->readAllStandardOutput();
    });
    m_process->start(javaExe, args);

    m_connectTimer.start();
}

void IceAdapterService::tryConnect()
{
    if (m_socket.state() == QAbstractSocket::UnconnectedState)
        m_socket.connectToHost("127.0.0.1", m_rpcPort);
}

void IceAdapterService::onSocketConnected()
{
    qInfo() << "[IceAdapterService::onSocketConnected] connected to RPC port" << m_rpcPort;
    m_connectTimer.stop();
    emit rpcConnected();
}

void IceAdapterService::stop()
{
    m_connectTimer.stop();
    m_socket.disconnectFromHost();
    if (m_process)
    {
        m_process->terminate();
        m_process->waitForFinished(3000);
        delete m_process;
        m_process = nullptr;
    }
    m_readBuf.clear();
    m_iceAdapterLogPath.clear();
}

void IceAdapterService::sendRpc(QString method, QJsonArray params)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "[IceAdapterService::sendRpc] not connected, dropping" << method;
        return;
    }
    QJsonObject req;
    req.insert("jsonrpc", "2.0");
    req.insert("id", ++m_rpcIdCounter);
    req.insert("method", method);
    req.insert("params", params);
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact) + '\n';
    m_socket.write(data);
    m_socket.flush();
}

void IceAdapterService::onSocketReadyRead()
{
    m_readBuf += m_socket.readAll();
    int newline;
    while ((newline = m_readBuf.indexOf('\n')) >= 0)
    {
        QByteArray line = m_readBuf.left(newline);
        m_readBuf.remove(0, newline + 1);
        if (line.trimmed().isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError)
        {
            qWarning() << "[IceAdapterService] JSON parse error:" << err.errorString() << line;
            continue;
        }
        QJsonObject obj = doc.object();
        QString method = obj.value("method").toString();
        QJsonArray params = obj.value("params").toArray();
        if (!method.isEmpty())
            handleNotification(method, params);
    }
}

void IceAdapterService::handleNotification(QString method, QJsonArray params)
{
    qInfo() << "[IceAdapterService::handleNotification]" << method << params;
    if (method == "onIceMsg")
    {
        // params: [localPlayerId, remotePlayerId, iceMsgString]
        int remoteId = params.at(1).toInt();
        QJsonValue msg = params.at(2);
        emit onIceMsg(remoteId, msg);
    }
    else if (method == "onConnected")
    {
        // params: [localPlayerId, remotePlayerId, connected]
        int remoteId   = params.at(1).toInt();
        bool connected = params.at(2).toBool();
        emit onConnected(remoteId, connected);
    }
    else if (method == "onConnectionStateChanged")
    {
        if (!params.isEmpty() && params.at(0).toString() == "Disconnected")
            emit gameConnectionLost();
    }
    else if (method == "onGpgNetMessageReceived")
    {
        QString command = params.at(0).toString();
        QJsonArray args = params.at(1).toArray();
        emit gpgNetMessageReceived(command, args);
    }
}

void IceAdapterService::hostGame(QString mapName)
{
    sendRpc("hostGame", QJsonArray{ mapName });
}

void IceAdapterService::joinGame(QString login, int remotePlayerId)
{
    sendRpc("joinGame", QJsonArray{ login, remotePlayerId });
}

void IceAdapterService::connectToPeer(QString login, int remotePlayerId, bool offer)
{
    sendRpc("connectToPeer", QJsonArray{ login, remotePlayerId, offer });
}

void IceAdapterService::disconnectFromPeer(int remotePlayerId)
{
    sendRpc("disconnectFromPeer", QJsonArray{ remotePlayerId });
}

void IceAdapterService::iceMsg(int remotePlayerId, QJsonValue msg)
{
    sendRpc("iceMsg", QJsonArray{ remotePlayerId, msg });
}

void IceAdapterService::setIceServers(QJsonArray iceServers)
{
    sendRpc("setIceServers", QJsonArray{ iceServers });
}

void IceAdapterService::setLobbyInitMode(QString mode)
{
    sendRpc("setLobbyInitMode", QJsonArray{ mode });
}
