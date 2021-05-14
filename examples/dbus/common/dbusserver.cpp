// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "dbusserver.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QThread>

#include <thread>

const QString DBusServer::serviceName = QStringLiteral("org.kde.qoro.dbustest");
const QString DBusServer::objectPath = QStringLiteral("/");
const QString DBusServer::interfaceName = QStringLiteral("org.kde.qoro.dbuserver");

DBusServer::DBusServer() {
    qInfo() << "DBusServer started";
    auto bus = QDBusConnection::sessionBus();
    bus.registerService(serviceName);
    bus.registerObject(objectPath, interfaceName, this, QDBusConnection::ExportAllSlots);
}

QString DBusServer::blockingPing(int seconds) const {
    qInfo() << "S: Received ping request...";
    std::this_thread::sleep_for(std::chrono::seconds{seconds});
    qInfo() << "S: sending PONG response";
    return QStringLiteral("PONG!");
}

std::unique_ptr<QProcess> DBusServer::runStadaloneServer() {
#ifdef SERVER_EXEC_PATH
    auto process = std::make_unique<QProcess>();
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(QStringLiteral(SERVER_EXEC_PATH), {}, QIODevice::ReadOnly);
    process->waitForStarted();
    if (process->state() != QProcess::Running) {
        qCritical() << "Failed to start server process:" << process->error();
    }
    return process;
#else
    return {};
#endif
}

#ifdef STANDALONE
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    DBusServer server;
    return app.exec();
}
#endif
