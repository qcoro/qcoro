#pragma once

#include "qcoroasyncsqlresult.h"

#include <QSqlError>
#include <QVariant>

namespace QCoro
{

class AsyncSqlDriver;
class AsyncSqlNullResult : public AsyncSqlResult
{
public:
    inline explicit AsyncSqlNullResult(const AsyncSqlDriver* d): AsyncSqlResult(d)
    {
        AsyncSqlResult::setLastError(
            QSqlError(QLatin1StringView("Driver not loaded"), QLatin1StringView("Driver not loaded"), QSqlError::ConnectionError));
    }
    ~AsyncSqlNullResult() override = default;
protected:
    inline QVariant data(int) override { return QVariant(); }
    inline Task<bool> reset (const QString&) override { co_return false; }
    inline Task<bool> fetch(int) override { co_return false; }
    inline Task<bool> fetchFirst() override { co_return false; }
    inline Task<bool> fetchLast() override { co_return false; }
    inline bool isNull(int) override { return false; }
    inline int size() override { return -1; }
    inline int numRowsAffected() override { return 0; }

    inline void setAt(int) override {}
    inline void setActive(bool) override {}
    inline void setLastError(const QSqlError&) override {}
    inline void setQuery(const QString&) override {}
    inline void setSelect(bool) override {}
    inline void setForwardOnly(bool) override {}

    inline Task<bool> exec() override { co_return false; }
    inline Task<bool> prepare(const QString&) override { co_return false; }
    inline Task<bool> safePrepare(const QString&) override { co_return false; }
    inline void bindValue(int, const QVariant&, QSql::ParamType) override {}
    inline void bindValue(const QString&, const QVariant&, QSql::ParamType) override {}
};

} // namespace QCoro