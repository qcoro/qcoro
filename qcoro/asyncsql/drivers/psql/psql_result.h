#pragma once

#include "qcoroasyncsqlresult.h"
#include "drivers/psql/psql_driver.h"

class QCoroAsyncPsqlDriver;
class QCoroAsyncPsqlResultPrivate;
class QCoroAsyncPsqlResult : public QCoro::AsyncSqlResult
{
public:
    QCoroAsyncPsqlResult(const QCoroAsyncPsqlDriver *driver);
    ~QCoroAsyncPsqlResult() override;

    QVariant handle() const override;
    void virtual_hook(int id, void *data) override;

protected:
    void cleanup();
    QCoro::Task<bool> fetch(int i) override;
    QCoro::Task<bool> fetchFirst() override;
    QCoro::Task<bool> fetchLast() override;
    QCoro::Task<bool> fetchNext() override;
    QCoro::Task<bool> nextResult() override;
    QVariant data(int i) override;
    bool isNull(int field) override;
    QCoro::Task<bool> reset(const QString &query) override;
    int size() override;
    int numRowsAffected() override;
    QCoro::Task<QSqlRecord> record() const override;
    QCoro::Task<QVariant> lastInsertId() const override;
    QCoro::Task<bool> prepare(const QString &query) override;
    QCoro::Task<bool> exec() override;

private:
    Q_DECLARE_PRIVATE(QCoroAsyncPsqlResult)
};