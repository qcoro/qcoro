// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
// SPDX-FileCopyrightText: 2016 The Qt Company Ltd.
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroasyncsql_export.h"
#include "qcorotask.h"

#include <qtsqlglobal.h>
#include <QSqlDriver>
#include <QObject>
#include <QString>
#include <QStringList>

class QSqlError;
class QSqlField;
class QSqlIndex;
class QSqlRecord;
class QVariant;

namespace QCoro
{

class AsyncSqlResult;
class AsyncSqlDriverPrivate;
class QCOROASYNCSQL_EXPORT AsyncSqlDriver : public QObject
{
    friend class AsyncSqlDatabase;
    friend class AsyncSqlResultPrivate;
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AsyncSqlDriver)
public:
    using DriverFeature = QSqlDriver::DriverFeature;
    using StatementType = QSqlDriver::StatementType;
    using IdentifierType = QSqlDriver::IdentifierType;
    using NotificationSource = QSqlDriver::NotificationSource;

    enum DbmsType {
        UnknownDbms,
        MySqlServer,
        PostgreSQL,
        SQLite,
    };

    Q_PROPERTY(QSql::NumericalPrecisionPolicy numericalPrecisionPolicy READ numericalPrecisionPolicy WRITE setNumericalPrecisionPolicy)

    explicit AsyncSqlDriver(QObject *parent = nullptr);
    ~AsyncSqlDriver();
    virtual bool isOpen() const;
    bool isOpenError() const;

    virtual Task<bool> beginTransaction();
    virtual Task<bool> commitTransaction();
    virtual Task<bool> rollbackTransaction();
    virtual Task<QStringList> tables(QSql::TableType tableType) const;
    virtual Task<QSqlIndex> primaryIndex(const QString &tableName) const;
    virtual Task<QSqlRecord> record(const QString &tableName) const;

    virtual QString formatValue(const QSqlField &field, bool trimStrings = false) const;
    virtual QString escapeIdentifier(const QString &identifier, IdentifierType type) const;
    virtual QString sqlStatement(StatementType type, const QString &tableName,
                                 const QSqlRecord &rec, bool preparedStatement) const;

    QSqlError lastError() const;

    virtual QVariant handle() const;
    virtual bool hasFeature(DriverFeature feature) const = 0;
    virtual Task<void> close() = 0;
    virtual AsyncSqlResult *createResult() const = 0;

    virtual Task<bool> open(const QString &db,
                            const QString &user = QString(),
                            const QString &password = QString(),
                            const QString &host = QString(),
                            int port = -1,
                            const QString &connOpts = QString()) = 0;

#if 0 // TODO: Notification support
    virtual Task<bool> subscribeToNotification(const QString &name);
    virtual Task<bool> unsubscribeFromNotification(const QString &name);
    virtual QStringList subscribedToNotifications() const;
#endif

    virtual bool isIdentifierEscaped(QStringView identifier, IdentifierType type) const;
    virtual QString stripDelimiters(QStringView identifier, IdentifierType type) const;

    void setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy);
    QSql::NumericalPrecisionPolicy numericalPrecisionPolicy() const;

    DbmsType dbmsType() const;
    virtual int maximumIdentifierLength(IdentifierType type) const;
public Q_SLOTS:
    virtual bool cancelQuery();

Q_SIGNALS:
    void notification(const QString &name, AsyncSqlDriver::NotificationSource source, const QVariant &payload);

protected:
    AsyncSqlDriver(AsyncSqlDriverPrivate *dd, QObject *parent = nullptr);
    virtual void setOpen(bool open);
    virtual void setOpenError(bool openError);
    virtual void setLastError(const QSqlError& error);

    AsyncSqlDriverPrivate *d_ptr;
private:
    Q_DECLARE_PRIVATE(AsyncSqlDriver)
};

} // namespace QCoro
