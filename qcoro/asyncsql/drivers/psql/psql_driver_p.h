#pragma once

#include "qcoroasyncsqldriver_p.h"
#include "psql_driver.h"
#include "psql_types.h"

#include <libpq-fe.h>

#include <QHash>
#include <qsqlerror.h>

struct Version
{
    static constexpr auto unknown() { return Version{0, 0}; }

    constexpr bool operator<(const Version &other) const
    {
        return major < other.major || (major == other.major && minor < other.minor);
    }

    constexpr bool operator<=(const Version &other) const
    {
        return major < other.major || (major == other.major && minor <= other.minor);
    }

    constexpr bool operator>(const Version &other) const
    {
        return major > other.major || (major == other.major && minor > other.minor);
    }

    constexpr bool operator>=(const Version &other) const
    {
        return major > other.major || (major == other.major && minor >= other.minor);
    }

    constexpr bool operator==(const Version &other) const
    {
        return major == other.major && minor == other.minor;
    }

    int major = 0;
    int minor = 0;
};

namespace QCoro
{
class AsyncSqlDriver;
class AsyncSqlQuery;
} // namespace QCoro

class QCoroAsyncPsqlDriverPrivate final : public QCoro::AsyncSqlDriverPrivate
{
public:
    explicit QCoroAsyncPsqlDriverPrivate();
    ~QCoroAsyncPsqlDriverPrivate() override = default;

    PGconn *conn = nullptr;
    Version psqlVersion = Version::unknown();
    QCoroPsql::StatementId currentStmtId = QCoroPsql::InvalidStatementId;
    QCoroPsql::StatementId stmtCount = QCoroPsql::InvalidStatementId;
    bool hasBackslashEscape = false;

    QCoro::Task<void> appendTables(QStringList &tables, QCoro::AsyncSqlQuery &query, QChar type);
    QCoro::Task<QCoroPsql::PGresultPtr> exec(const QString &query);
    QCoro::Task<QCoroPsql::PGresultPtr> exec(const char *stmt);
    QCoro::Task<QCoroPsql::StatementId> sendQuery(const QString &stmt);
    bool setSingleRowMode() const;
    QCoro::Task<QCoroPsql::PGresultPtr> getResult() const;
    QCoro::Task<QCoroPsql::PGresultPtr> getResult(QCoroPsql::StatementId stmtId) const;
    void finishQuery(QCoroPsql::StatementId stmtId);
    void discardResults() const;
    QCoroPsql::StatementId generateStatementId();

    bool setNonblockingConnection();
    bool detectServerVersion();
    QCoro::Task<bool> setEncodingUtf8();
    QCoro::Task<bool> setDateStyle();
    QCoro::Task<bool> setByteaOutput();
    QCoro::Task<bool> setUtcTimeZone();
    QCoro::Task<bool> detectBackslashEscape();

    void setLastError(const QSqlError &error) const
    {
        Q_Q(const QCoroAsyncPsqlDriver);
        const_cast<QCoroAsyncPsqlDriver *>(q)->setLastError(error);
    }

    mutable QHash<int, QString> oidToTable = {};

private:
    Q_DECLARE_PUBLIC(QCoroAsyncPsqlDriver)
};

