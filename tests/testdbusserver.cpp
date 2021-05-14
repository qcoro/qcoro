// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testdbusserver.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include <QTimer>

#include <thread>

DBusServer::DBusServer() {
    QTimer::singleShot(0, this, &DBusServer::run);
}

void DBusServer::run() {
    auto conn = QDBusConnection::sessionBus();
    if (!conn.registerService(serviceName)) {
        qWarning() << "Failed to register service to DBus:" << conn.lastError().message();
    }
    if (!conn.registerObject(objectPath, interfaceName, this, QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register object to DBus" << conn.lastError().message();
    }
}

void DBusServer::foo() {}

void DBusServer::blockFor(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

QString DBusServer::blockAndReturn(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return QStringLiteral("Slept for %1 seconds").arg(seconds);
}

QString DBusServer::ping(const QString &ping) {
    return ping;
}

void DBusServer::quit() {
    qApp->quit();
}
