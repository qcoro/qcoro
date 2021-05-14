// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QProcess>

#include <memory>

class DBusServer : public QObject {
    Q_OBJECT
public:
    static const QString serviceName;
    static const QString objectPath;
    static const QString interfaceName;

    explicit DBusServer();

    static std::unique_ptr<QProcess> runStadaloneServer();
public Q_SLOTS:
    QString blockingPing(int seconds) const;
};
