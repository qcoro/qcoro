// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
// SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroasyncsql_export.h"
#include "qcorotask.h"

#include <qtsqlglobal.h>
#include <QObject>

class QSqlError;
class QSqlIndex;
class QSqlRecord;

namespace QCoro
{

class AsyncSqlDriver;
class AsyncSqlDatabasePrivate;


class QCOROASYNCSQL_EXPORT AsyncSqlDriverCreatorBase
{
public:
    virtual ~AsyncSqlDriverCreatorBase() = default;
    virtual AsyncSqlDriver *createDriver() const = 0;
};

class QCOROASYNCSQL_EXPORT AsyncSqlDatabase
{
    Q_GADGET
public:
    Q_PROPERTY(QSql::NumericalPrecisionPolicy numericalPrecisionPolicy READ numericalPrecisionPolicy WRITE setNumericalPrecisionPolicy)

    explicit AsyncSqlDatabase();
    AsyncSqlDatabase(const AsyncSqlDatabase &other);
    ~AsyncSqlDatabase();

    AsyncSqlDatabase &operator=(const AsyncSqlDatabase &other);

    Task<bool> open();
    Task<bool> open(const QString& user, const QString& password);
    Task<void> close();
    bool isOpen() const;
    bool isOpenError() const;
    Task<QStringList> tables(QSql::TableType type = QSql::Tables) const;
    Task<QSqlIndex> primaryIndex(const QString& tablename) const;
    Task<QSqlRecord> record(const QString& tablename) const;

    QSqlError lastError() const;
    bool isValid() const;

    Task<bool> transaction();
    Task<bool> commit();
    Task<bool> rollback();

    void setDatabaseName(const QString& name);
    void setUserName(const QString& name);
    void setPassword(const QString& password);
    void setHostName(const QString& host);
    void setPort(int port);
    void setConnectOptions(const QString& options = QString());
    QString databaseName() const;
    QString userName() const;
    QString password() const;
    QString hostName() const;
    QString driverName() const;
    int port() const;
    QString connectOptions() const;
    QString connectionName() const;
    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

    AsyncSqlDriver* driver() const;

    static const char *defaultConnection;

    static AsyncSqlDatabase addDatabase(const QString& type,
                                        const QString& connectionName = QLatin1StringView(defaultConnection));
    static AsyncSqlDatabase addDatabase(AsyncSqlDriver* driver,
                                        const QString& connectionName = QLatin1StringView(defaultConnection));
    static AsyncSqlDatabase cloneDatabase(const AsyncSqlDatabase &other, const QString& connectionName);
    static AsyncSqlDatabase cloneDatabase(const QString &other, const QString& connectionName);
    static Task<AsyncSqlDatabase> database(const QString& connectionName = QLatin1StringView(defaultConnection),
                                           bool open = true);
    static void removeDatabase(const QString& connectionName);
    static bool contains(const QString& connectionName = QLatin1StringView(defaultConnection));
    static QStringList drivers();
    static QStringList connectionNames();

    static bool isDriverAvailable(const QString &name);

protected:
    explicit AsyncSqlDatabase(const QString& type);
    explicit AsyncSqlDatabase(AsyncSqlDriver* driver);

private:
    friend class AsyncSqlDatabasePrivate;
    AsyncSqlDatabasePrivate *d;
};

} // namespace QCoro