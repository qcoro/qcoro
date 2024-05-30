// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroasyncsqldriver.h"

#include <QSqlIndex>
#include <QSqlRecord>
#include <QVariant>

namespace QCoro
{

class AsyncSqlNullDriver : public AsyncSqlDriver
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AsyncSqlNullDriver)
public:
    explicit AsyncSqlNullDriver(QObject *parent = nullptr): AsyncSqlDriver(parent) {}
    ~AsyncSqlNullDriver() override = default;

    bool isOpen() const override { return false; }
    Task<bool> beginTransaction() override { co_return false; }
    Task<bool> commitTransaction() override { co_return false; }
    Task<bool> rollbackTransaction() override { co_return false; }
    Task<QStringList> tables(QSql::TableType tableType) const override { co_return {}; }
    Task<QSqlIndex> primaryIndex([[maybe_unused]] const QString &tableName) const override { co_return QSqlIndex{}; }
    Task<QSqlRecord> record([[maybe_unused]] const QString &tableName) const override { co_return QSqlRecord{}; }

    QString formatValue([[maybe_unused]] const QSqlField &field, [[maybe_unused]] bool trimStrings) const override { return {}; }
    QString escapeIdentifier([[maybe_unused]] const QString &identifier, [[maybe_unused]] IdentifierType type) const override { return {}; }
    QString sqlStatement([[maybe_unused]] StatementType type, [[maybe_unused]] const QString &tableName,
                         [[maybe_unused]] const QSqlRecord &rec, [[maybe_unused]] bool preparedStatement) const override { return {}; }

    QVariant handle() const override { return {}; }
    bool hasFeature([[maybe_unused]] DriverFeature feature) const override { return false; }
    Task<void> close() override { co_return; }
    AsyncSqlResult *createResult() const override { return nullptr; }

    Task<bool> open([[maybe_unused]] const QString &db, [[maybe_unused]] const QString &user,
                    [[maybe_unused]] const QString &password, [[maybe_unused]] const QString &host,
                    [[maybe_unused]] int port, [[maybe_unused]] const QString &connOpts) override {
        co_return false;
    }

#if 0 // TODO: Notification support
    Task<bool> subscribeToNotification([[maybe_unused]] const QString &name) override { co_return false; }
    Task<bool> unsubscribeFromNotification([[maybe_unused]] const QString &name) override { co_return false; }
#endif
};

} // namespace QCoro