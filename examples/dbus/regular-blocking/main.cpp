#include "common/dbusserver.h"

#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

void dbusWorker()
{
    auto bus = QDBusConnection::sessionBus();
    auto iface = QDBusInterface{DBusServer::serviceName, DBusServer::objectPath, DBusServer::interfaceName, bus};
    qInfo() << "Sending PING";
    QDBusReply<QString> response = iface.call(QStringLiteral("ping"));
    qInfo() << "Received response:" << response.value();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    DBusServer server;
    std::thread serverThread{[&server]{ server.run(); }};

    QTimer tickTimer;
    QObject::connect(&tickTimer, &QTimer::timeout, &app, []() {
        std::cout << QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toStdString() << " Tick!" << std::endl;
    });
    tickTimer.start(100ms);

    QTimer dbusTimer;
    QObject::connect(&dbusTimer, &QTimer::timeout, &app, dbusWorker);
    dbusTimer.start(5s);

    return app.exec();
}

