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

#include <chrono>

using namespace std::chrono_literals;

DBusServer::DBusServer() {
    QTimer::singleShot(0, this, &DBusServer::run);
    // Self-terminate if there's no interaction from a client in 30 seconds.
    // Prevents leaking the test server in case the test crashes.
    mSuicideTimer.setInterval(30s);
    mSuicideTimer.setSingleShot(true);
    connect(&mSuicideTimer, &QTimer::timeout, this, []() {
        std::cerr << "No call in 30 seconds, terminating!" << std::endl;
        qApp->exit(1);
    });
}

void DBusServer::run() {
    auto conn = QDBusConnection::sessionBus();
    if (!conn.registerService(serviceName)) {
        qWarning() << "Failed to register service to DBus:" << conn.lastError().message();
    }
    if (!conn.registerObject(objectPath, interfaceName, this, QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register object to DBus" << conn.lastError().message();
    }

    mSuicideTimer.start();
}

void DBusServer::foo() {
    mSuicideTimer.start();
}

void DBusServer::blockFor(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    mSuicideTimer.start();
}

QString DBusServer::blockAndReturn(int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    mSuicideTimer.start();
    return QStringLiteral("Slept for %1 seconds").arg(seconds);
}

QString DBusServer::blockAndReturnMultipleArguments(int seconds, bool &out) {
    std::this_thread::sleep_for(std::chrono::seconds{seconds});
    mSuicideTimer.start();
    out = true;
    return QStringLiteral("Hello World!");
}

QString DBusServer::ping(const QString &ping) {
    mSuicideTimer.start();
    return ping;
}

void DBusServer::quit() {
    mSuicideTimer.stop();
    qApp->quit();
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    DBusServer server;
    return app.exec();
}


