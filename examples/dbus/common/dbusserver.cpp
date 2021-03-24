#include "dbusserver.h"

#include <QCoreApplication>
#include <QDebug>

#include <thread>

const QString DBusServer::serviceName = QStringLiteral("org.kde.qoro.dbustest");
const QString DBusServer::objectPath = QStringLiteral("/");
const QString DBusServer::interfaceName = QStringLiteral("org.kde.qoro.dbuserver");

DBusServer::DBusServer(QObject *parent)
    : QObject{parent}
{

}

QString DBusServer::blockingPing(int seconds) const
{
    qInfo() << "S: Received ping request...";
    std::this_thread::sleep_for(std::chrono::seconds{seconds});
    qInfo() << "S: sending PONG response";
    return QStringLiteral("PONG!");
}

void DBusServer::run()
{
    qInfo() << "Starting server thread";
    DBusServer server;

    QEventLoop el;
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, &el, &QEventLoop::quit);
    el.exec();
}
