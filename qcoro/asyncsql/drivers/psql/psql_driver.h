#pragma once

#include "qcoroasyncsqldriver.h"

class QCoroAsyncPsqlDriverPrivate;
class QCoroAsyncPsqlDriver : public QCoro::AsyncSqlDriver
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QCoroAsyncPsqlDriver)
public:
    explicit QCoroAsyncPsqlDriver(QObject *parent = nullptr);
    ~QCoroAsyncPsqlDriver() override;

    bool isOpen() const override;
    QCoro::Task<bool> beginTransaction() override;
    QCoro::Task<bool> commitTransaction() override;
    QCoro::Task<bool> rollbackTransaction() override;
    QCoro::Task<QStringList> tables(QSql::TableType tableType) const override;
    QCoro::Task<QSqlIndex> primaryIndex(const QString &tableName) const override;
    QCoro::Task<QSqlRecord> record(const QString &tableName) const override;

    QString formatValue(const QSqlField &field, bool trimStrings) const override;
    QString escapeIdentifier(const QString &identifier, IdentifierType type) const override;

    QVariant handle() const override;
    bool hasFeature(DriverFeature feature) const override;
    QCoro::Task<void> close() override;
    QCoro::AsyncSqlResult *createResult() const override;

    QCoro::Task<bool> open(const QString &db, const QString &user, const QString &password,
                           const QString &host, int port, const QString &connOpts) override;
#if 0 // TODO: Notification support
    QCoro::Task<bool> subscribeToNotification(const QString &name) override;
    QCoro::Task<bool> unsubscribeFromNotification(const QString &name) override;
#endif

private:
    Q_DECLARE_PRIVATE(QCoroAsyncPsqlDriver)
};