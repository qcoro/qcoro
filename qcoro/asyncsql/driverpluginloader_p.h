// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QMap>
#include <QPluginLoader>

#include <memory>

namespace QCoro {

class AsyncSqlDriver;
class DriverPluginLoader
{
public:
    DriverPluginLoader(const char *pluginId, const QString &pluginPath);

    QList<QString> availableDrivers() const { return mPlugins.keys(); }

    AsyncSqlDriver *loadDriver(const QString &type);

private:
    void findPlugins();

    QByteArray mPluginId;
    QString mPluginPath;

    QMap<QString, std::shared_ptr<QPluginLoader>> mPlugins;
};

} // namespace QCoro