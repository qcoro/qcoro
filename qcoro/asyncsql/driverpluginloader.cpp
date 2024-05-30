// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "driverpluginloader_p.h"
#include "qcoroasyncsqldriverplugin_p.h"
#include "qcoroasyncsql_log.h"

#include <QPluginLoader>
#include <QCoreApplication>
#include <QDirIterator>
#include <QJsonObject>
#include <QJsonArray>


using namespace QCoro;

DriverPluginLoader::DriverPluginLoader(const char *pluginId, const QString &pluginPath)
    : mPluginId(pluginId), mPluginPath(pluginPath)
{
    findPlugins();
}

AsyncSqlDriver *DriverPluginLoader::loadDriver(const QString &type)
{
    auto plugin = mPlugins.find(type);
    if (plugin == mPlugins.end()) {
        return nullptr;
    }

    auto loader = *plugin;
    if (!loader->isLoaded()) {
        if (!loader->load()) {
            qCWarning(AsyncSqlLog) << "Failed to load plugin" << loader->fileName() << ":" << loader->errorString();
            return nullptr;
        }
    }

    auto instance = loader->instance();
    // FIXME: qobject_cast is failing here for some reason
    auto pluginInstance = dynamic_cast<QCoro::AsyncSqlDriverPlugin *>(instance);
    Q_ASSERT(pluginInstance != nullptr);

    return pluginInstance->create(type);
}

void DriverPluginLoader::findPlugins()
{
    mPlugins.clear();

    const auto paths = QCoreApplication::libraryPaths();
    for (const auto &basePath : paths) {
        const auto path = basePath + QLatin1Char('/') + mPluginPath;
        QDirIterator scan(path, QDir::Files);
        while (scan.hasNext()) {
            const auto filePath = scan.nextFileInfo().absoluteFilePath();
            if (!QLibrary::isLibrary(filePath)) {
                continue;
            }

            const auto loader = std::make_shared<QPluginLoader>(filePath);
            const auto metadata = loader->metaData();
            if (metadata.value(u"IID") != QLatin1String(mPluginId.data(), mPluginId.size())) {
                continue;
            }

            const auto metadataMap = metadata.value(u"MetaData").toObject();
            auto keys = metadataMap.value(u"Keys").toArray();
            for (const auto &key : keys) {
                mPlugins.insert(key.toString(), loader);
            }
        }
    }
}